#include "RefinerTetra.hpp"
#include "Output.hpp"
#include "Cutter3D.hpp"

using namespace MainApplication;

namespace GeDiM
{
    RefinerTetra::RefinerTetra()
    {

    }
    RefinerTetra::~RefinerTetra()
    {

    }
    const Output::ExitCodes RefinerTetra::AddIdCell(const unsigned int& idCell)
	{
		idCellToRefine.push_back(idCell);
		return Output::Success;
	}

    const GenericEdge* RefinerTetra::FindMaxEdge(GenericCell& cell)
    {
        //ricerca lato pi� lungo
        double max = 0;
        unsigned int edge_pos;
        for(int i = 0; i < cell.NumberOfEdges(); i++)
        {
            const Vector3d first_point = cell.Edge(i) -> Point(0) -> Coordinates();
            const Vector3d second_point = cell.Edge(i) -> Point(1) -> Coordinates();
            double length = (second_point - first_point).norm();
            if(max < length)
            {
                max = length;
                edge_pos = i;
            }
        }
        return cell.Edge(edge_pos);
    }

    const Output::ExitCodes RefinerTetra::CutTetra(GenericCell& cell)
    {
        //Controllo che non sia già stato tagliato
        if(cell.IsActive())
        {

            //cerco il lato da tagliare
            GenericEdge* MaxEdge = meshPointer->Edge(FindMaxEdge(*meshPointer->Cell(cell.Id()))->Id());
            //trovo il punto medio
            /*Vector3d point = 0.5 * (edge_to_split -> Point(0) ->Coordinates() + edge_to_split -> Point(1) ->Coordinates());

            cutter.Reset();
            cutter.SetMesh(*meshPointer);
            Vector3d a[2];
            int j = 0;
            for(int i = 0; i < cell.NumberOfPoints() ; i++)
            {
                const GenericPoint* current_point = cell.Point(i);
                if(current_point != edge_to_split->Point(0) && current_point != edge_to_split->Point(1))
                {
                    a[j] = current_point -> Coordinates() - point;
                    j++;
                }
            }
            Vector3d normal = a[0].cross(a[1]);
            normal.normalize();
            double translation = normal.dot(point);
            cutter.intersector2D1D = new Intersector2D1D;
            Intersector2D1D& intersector = *cutter.intersector2D1D;
			intersector.SetPlane(normal, translation);


            if(cutter.CutCell(cell, normal, translation) == Output::Success)
            {
                cout << "\nFirst cut successful"  << endl;

                if(RecoverConformity(cell, point, *edge_to_split) == Output::Success)
                {
                    cout << "Recover Conformity successful"  << endl;
                }
            }*/
            const Vector3d c_point = 0.5 * ( MaxEdge-> Point(0) ->Coordinates() + MaxEdge -> Point(1) ->Coordinates());
            //  CREAZIONE DEL PUNTO MEDIO
            const vector<Vector3d> x = {c_point};
            meshPointer->CutEdgeWithPoints(MaxEdge->Id(), x);
            unsigned int pos_middle = meshPointer->NumberOfPoints()-1;
            RecoverConformity(*MaxEdge, *meshPointer->Point(pos_middle));

        }
        return Output::Success;
    }

    const Output::ExitCodes RefinerTetra::RecoverConformity(const GenericEdge& long_edge, const GenericPoint& middlePoint)
    {
        //TROVO a, b
        GenericEdge* a;
        GenericEdge* b;
        if(meshPointer->Edge(long_edge.Child(0)->Id())->Point(0) == long_edge.Point(0) || meshPointer->Edge(long_edge.Child(0)->Id())->Point(1) == long_edge.Point(0))
        {
            a = meshPointer->Edge(long_edge.Child(0)->Id());
            b = meshPointer->Edge(long_edge.Child(1)->Id());
        }else
        {
            b = meshPointer->Edge(long_edge.Child(0)->Id());
            a = meshPointer->Edge(long_edge.Child(1)->Id());
        }
        //CICLO NELLE CELLE CON IL LATO CHE E' STATO TAGLIATO
        for (int i = 0; i < long_edge.NumberOfCells(); i++)
        {
            //CREAZIONE VETTORI PER MEMORIZZAZIONE TETRAEDRO
            vector <const GenericPoint*> points;
            vector <const GenericEdge*> edges;
            vector <GenericEdge*> new_edges;
            vector <const GenericFace*> faces;
            vector <GenericFace*> new_faces;
            vector <GenericCell*> cells;

            const GenericCell& current_cell = *long_edge.Cell(i);
            if(current_cell.IsActive())
            {
                cout << "Nuova cella da rifinire\n" << endl;
                if(CellIntegrityCheck(current_cell.Id()) == Output::Success)
                    cout<<"Cell Integrity correct\n";
                cout << "ID " << current_cell.Id() << "\n" << endl;
                unsigned int counter = 0;
                //CICLO NELLE FACCIE VICINE AL LATO TAGLIATO
                for(int j = 0; j < 4; j++)
                {
                    const GenericFace& current_face = *current_cell.Face(j);
                    //CICLO NELLE FACCIE DEL TETRAEDRO CORRENTE
                    if(current_face.Edge(0) == &long_edge || current_face.Edge(1) == &long_edge || current_face.Edge(2) == &long_edge)
                    {
                        //CONTROLLO FACCIA SE GIA' TAGLIATA E SE E' UNA DELLE DUE CHE CI INTERESSANO
                        if(!current_face.HasChilds())
                        {
                            counter++;
                            cout << "\tFaccia da tagliare numero " << counter << "\n" << endl;
                            GenericFace& current_child = *meshPointer->Face(current_face.Id());
                            faces.push_back(&current_child);
                            //RICERCA DEL PUNTO OPPOSTO A MIDDLEPOINT
                            unsigned int pos_point = points.size();  //INDICE POSIZIONE NEL VETTORE POINTS DEL PUNTO OPPOSTO A MIDDLE POINT
                            for(int x = 0; x < 3; x++)
                            {
                                if(current_child.Point(x) != long_edge.Point(0) && current_child.Point(x) != long_edge.Point(1) && current_child.Point(x) != &middlePoint)
                                {
                                    points.push_back(current_child.Point(x));
                                }
                            }
                            //creazione nuovo edge
                            unsigned int pos_edge = new_edges.size(); //INDICE POSIZIONE NEL VETTORE new_edges DEL NUOVO LATO CREATO
                            new_edges.push_back(meshPointer->CreateEdge());
                            new_edges[pos_edge]->AddPoint(&middlePoint);
                            new_edges[pos_edge]->AddPoint(points[pos_point]);
                            meshPointer->AddEdge(new_edges[pos_edge]);

                            //Creazione nuove facce
                            unsigned int pos_new_faces = new_faces.size(); //INDICE POSIZIONE NEL VETTORE new_faces DELLA PRIMA DELLE FACCE CREATE
                            unsigned int pos_faces = faces.size() -1;  //INDICE POSIZIONE NEL VETTORE faces DELLA FACCIA CHE STIAMO TAGLIANDO
                            new_faces.push_back(meshPointer->CreateFace());
                            new_faces.push_back(meshPointer->CreateFace());

                            //deattivazione faccia
                            meshPointer->Face(faces[pos_faces]->Id())->SetState(false);
                            meshPointer->Face(faces[pos_faces]->Id())->InitializeChilds(2);
                            meshPointer->Face(faces[pos_faces]->Id())->AddChild(new_faces[pos_new_faces]);
                            meshPointer->Face(faces[pos_faces]->Id())->AddChild(new_faces[pos_new_faces +1]);

                            //settaggio padre
                            new_faces[pos_new_faces]->SetFather(faces[pos_faces]);
                            new_faces[pos_new_faces + 1]->SetFather(faces[pos_faces]);

                            //AGGIUNTA PUNTI ALLE FACCE si può fare in un ciclo
                            new_faces[pos_new_faces]->InitializePoints(3);
                            new_faces[pos_new_faces +1]->InitializePoints(3);
                            new_faces[pos_new_faces]->AddPoint(&middlePoint);
                            new_faces[pos_new_faces]->AddPoint(points[pos_point]);
                            new_faces[pos_new_faces]->AddPoint(long_edge.Point(0));
                            new_faces[pos_new_faces +1]->AddPoint(&middlePoint);
                            new_faces[pos_new_faces +1]->AddPoint(points[pos_point]);
                            new_faces[pos_new_faces +1]->AddPoint(long_edge.Point(1));

                            //AGGIUNTA LATI ALLE FACCE
                            new_faces[pos_new_faces]->InitializeEdges(3);
                            new_faces[pos_new_faces +1]->InitializeEdges(3);
                            new_faces[pos_new_faces]->AddEdge(a);
                            new_faces[pos_new_faces]->AddEdge(new_edges[pos_edge]);
                            new_faces[pos_new_faces +1]->AddEdge(b);
                            new_faces[pos_new_faces +1]->AddEdge(new_edges[pos_edge]);
                            //RICERCA g, e
                            GenericEdge* g;
                            GenericEdge* e;
                            for(int y = 0; y < 3; y++)
                            {
                                if(faces[pos_faces]->Edge(y) != &long_edge)
                                {
                                    if(faces[pos_faces]->Edge(y)->Point(0) == long_edge.Point(0) || faces[pos_faces]->Edge(y)->Point(1) == long_edge.Point(0))
                                    {
                                        g = meshPointer->Edge(faces[pos_faces]->Edge(y)->Id());
                                        new_faces[pos_new_faces]->AddEdge(g);
                                    }else
                                    {
                                        e = meshPointer->Edge(faces[pos_faces]->Edge(y)->Id());
                                        new_faces[pos_new_faces +1]->AddEdge(e);
                                    }
                                }
                            }
                            edges.push_back(g);
                            edges.push_back(e);

                            meshPointer->AddFace(new_faces[pos_new_faces]);
                            meshPointer->AddFace(new_faces[pos_new_faces +1]);

                            //AGGIORNO VICINI c E d
                            new_edges[pos_edge]->AddFace(new_faces[pos_new_faces]);
                            new_edges[pos_edge]->AddFace(new_faces[pos_new_faces +1]);

                            //AGGIORNO VICINI E e D
                            meshPointer->Point(points[pos_point]->Id())->AddFace(new_faces[pos_new_faces]);
                            meshPointer->Point(points[pos_point]->Id())->AddFace(new_faces[pos_new_faces +1]);

                            meshPointer->Point(points[pos_point]->Id())->AddEdge(new_edges[pos_edge]);
                            //AGGIORNO VICINI MIDDLEPOINT
                            meshPointer->Point(middlePoint.Id())->AddEdge(new_edges[pos_edge]);
                            meshPointer->Point(middlePoint.Id())->AddFace(new_faces[pos_new_faces]);
                            meshPointer->Point(middlePoint.Id())->AddFace(new_faces[pos_new_faces +1]);
                            //AGGIORNO a E b
                            a->AddFace(new_faces[pos_new_faces]);
                            a->AddEdge(new_edges[pos_edge]);
                            b->AddFace(new_faces[pos_new_faces +1]);
                            b->AddEdge(new_edges[pos_edge]);
                            //AGGIORNO VICNI NUOVE FACCE
                            new_faces[pos_new_faces]->AddFace(new_faces[pos_new_faces +1]);
                            new_faces[pos_new_faces +1]->AddFace(new_faces[pos_new_faces]);
                            //AGGIORNO g, e oppure i, f
                            unsigned int x = edges.size()-1;

                            meshPointer->Edge(edges[x-1]->Id())->AddFace(new_faces[pos_new_faces]);
                            meshPointer->Edge(edges[x]->Id())->AddFace(new_faces[pos_new_faces +1]);
                        }
                        ///CASO IN CUI LA FACCIA SIA GIA' STATA TAGLIATA
                        else
                        {
                            counter++;
                            //AGGIUNTA FACCIA SE GIA' TAGLIATA

                            cout << "\tFaccia gia' tagliata numero " << counter << "\n" << endl;

                            unsigned int pos_faces = faces.size();
                            unsigned int pos_new_faces = new_faces.size();
                            faces.push_back(&current_face);
                            if(current_face.NumberOfChilds() != 2)
                                cout << "Error: face with "<<current_face.NumberOfChilds()<<" childs\n";
                            if(meshPointer->Face(current_face.Child(0)->Id())->Edge(0) == a || meshPointer->Face(current_face.Child(0)->Id())->Edge(1) == a || meshPointer->Face(current_face.Child(0)->Id())->Edge(2) == a)
                            {
                                new_faces.push_back(meshPointer->Face(current_face.Child(0)->Id()));
                                new_faces.push_back(meshPointer->Face(current_face.Child(1)->Id()));
                            }else
                            {
                                new_faces.push_back(meshPointer->Face(current_face.Child(1)->Id()));
                                new_faces.push_back(meshPointer->Face(current_face.Child(0)->Id()));
                            }
                            //RICERCA E oppure D
                            unsigned int pos_point = points.size();
                            for(int x = 0; x < 3; x++)
                            {
                                if(new_faces[pos_new_faces]->Point(x) != long_edge.Point(0) && new_faces[pos_new_faces]->Point(x) != &middlePoint)
                                {
                                    points.push_back(new_faces[pos_new_faces]->Point(x));
                                }
                            }
                            //RICERCA DI c oppure d
                            unsigned int pos_new_edges = new_edges.size();
                            for (int y = 0; y < 3; y++)
                            {
                                if (new_faces[pos_new_faces]->Edge(y)->Point(0) == &middlePoint || new_faces[pos_new_faces]->Edge(y)->Point(1) == &middlePoint)
                                {
                                    if(new_faces[pos_new_faces]->Edge(y)->Point(0) == points[pos_point] || new_faces[pos_new_faces]->Edge(y)->Point(1) == points[pos_point])
                                    {
                                        new_edges.push_back(meshPointer->Edge(new_faces[pos_new_faces]->Edge(y)->Id()));
                                    }
                                }
                            }
                            unsigned int count_ge = 0;
                            for(int y = 0; y < 3; y++)//RICERCA DI g
                            {
                                if(new_faces[pos_new_faces]->Edge(y) != a && new_faces[pos_new_faces]->Edge(y) != new_edges[pos_new_edges])
                                {
                                    edges.push_back(meshPointer->Edge(new_faces[pos_new_faces]->Edge(y)->Id()));
                                    count_ge++;
                                }
                            }
                            if(count_ge != 1)
                                cout << "ERROR: too many g edges\n";
                            count_ge = 0;
                            int count_b = 0;
                            int count_new_edge = 0;
                            for(int y = 0; y < 3; y++)//RICERCA DI e
                            {
                                if(new_faces[pos_new_faces +1]->Edge(y) != b && new_faces[pos_new_faces +1]->Edge(y) != new_edges[pos_new_edges])
                                {
                                    edges.push_back(meshPointer->Edge(new_faces[pos_new_faces +1]->Edge(y)->Id()));
                                    count_ge++;
                                }
                                if(new_faces[pos_new_faces +1]->Edge(y) == b)
                                    count_b++;
                                if(new_faces[pos_new_faces +1]->Edge(y) == new_edges[pos_new_edges])
                                    count_new_edge++;
                            }
                            if(count_ge != 1)
                                cout << "ERROR: too many e edges "<< count_b << count_new_edge << endl;

                        }
                    }
                }
                ///CONTROLLI PER INTEGRITA' VETTORI E PROCESSO
                if(counter != 2)
                    {
                        return Output::GenericError;
                    }
                if(points.size() != 2)
                    cout << "ERROR: Size of points = "<< points.size()<<"\n";
                if(new_edges.size() != 2)
                    cout << "ERROR: Size of new_edges = "<< new_edges.size()<<"\n";
                if(new_faces.size() != 4)
                    cout << "ERROR: Size of new_faces = "<< new_faces.size()<<"\n";
                if(edges.size() != 4)
                    cout << "ERROR: Size of edges = "<< edges.size()<<"\n";
                if(faces.size() != 2)
                    cout << "ERROR: Size of faces = "<< faces.size()<<"\n";

                //TROVO LATO h
                for(int j=0; j<6; j++)
                {
                    if(current_cell.Edge(j)->Point(0) == points[0] || current_cell.Edge(j)->Point(1) == points[0])
                    {
                        if(current_cell.Edge(j)->Point(0) == points[1] || current_cell.Edge(j)->Point(1) == points[1])
                        {
                            edges.push_back(current_cell.Edge(j));
                        }
                    }
                }
                //RICERCA DELLE FACCIE C e F
                for(int j = 0; j < 4; j++)
                {
                    faces.resize(4);
                    if(current_cell.Face(j)->Edge(0) == edges[4] || current_cell.Face(j)->Edge(1) == edges[4] || current_cell.Face(j)->Edge(2) == edges[4])
                    {
                        if(current_cell.Face(j)->Point(0) == long_edge.Point(0) || current_cell.Face(j)->Point(1) == long_edge.Point(0) || current_cell.Face(j)->Point(2) == long_edge.Point(0))
                        {
                            faces[2] = current_cell.Face(j);
                        }else
                        {
                            faces[3] = current_cell.Face(j);
                        }

                    }
                }

                //creo faccia di taglio G
                new_faces.push_back(meshPointer->CreateFace());
                new_faces[4]->AddPoint(&middlePoint);
                new_faces[4]->AddPoint(points[0]);
                new_faces[4]->AddPoint(points[1]);
                new_faces[4]->AddEdge(new_edges[0]);
                new_faces[4]->AddEdge(new_edges[1]);
                new_faces[4]->AddEdge(edges[4]);
                meshPointer->AddFace(new_faces[4]);

                //CREAZIONE NUOVE CELLE
                for(int j=0; j<2; j++)
                {
                    //Creazione nuovi tetraedri
                    cells.push_back(meshPointer->CreateCell());
                    //aggiunta punti
                    cells[j]->AddPoint(points[0]);
                    cells[j]->AddPoint(points[1]);
                    cells[j]->AddPoint(long_edge.Point(j));
                    cells[j]->AddPoint(&middlePoint);
                    //aggiunta edges
                    cells[j]->AddEdge(new_edges[0]);
                    cells[j]->AddEdge(new_edges[1]);
                    cells[j]->AddEdge(edges[4]);
                    cells[j]->AddEdge(edges[j]);
                    cells[j]->AddEdge(edges[j+2]);
                    //aggiunta facce
                    cells[j]->AddFace(new_faces[4]);
                    cells[j]->AddFace(new_faces[j]);
                    cells[j]->AddFace(new_faces[j+2]);
                    cells[j]->AddFace(faces[j+2]);

                    meshPointer->AddCell(cells[j]);
                    meshPointer->Cell(current_cell.Id())->AddChild(cells[j]);
                    cells[j]->SetFather(&current_cell);
                }
                cells[0]->AddEdge(a);
                cells[1]->AddEdge(b);
                meshPointer->Cell(current_cell.Id())->SetState(false);

                //AGGIUNTA VICINI h
                meshPointer->Edge(edges[4]->Id())->AddCell(cells[0]);
                meshPointer->Edge(edges[4]->Id())->AddCell(cells[1]);
                meshPointer->Edge(edges[4]->Id())->AddFace(new_faces[4]);

                //AGGIUNTA VICINI E e D (vertici)
                for(int j =0; j<2; j++)
                {
                    meshPointer->Point(points[j]->Id())->AddCell(cells[0]);
                    meshPointer->Point(points[j]->Id())->AddCell(cells[1]);
                }

                //AGGIUNTA VICINI MIDDELPOINT
                meshPointer->Point(middlePoint.Id())->AddCell(cells[0]);
                meshPointer->Point(middlePoint.Id())->AddCell(cells[1]);
                //AGGIUNTA VICINI a E b
                a->AddCell(cells[0]);
                b->AddCell(cells[1]);


                //AGGIUNTA VICINI c E d
                for(int j=0; j<2; j++)
                {
                    new_edges[j]->AddCell(cells[0]);
                    new_edges[j]->AddCell(cells[1]);
                    new_edges[j]->AddFace(new_faces[4]);
                }
                //AGGIUNTA VICINI G
                new_faces[4]->AddCell(cells[0]);
                new_faces[4]->AddCell(cells[1]);
                for(int j=0; j<4; j++)
                {
                    new_faces[4]->AddFace(new_faces[j]);
                }
                new_faces[4]->AddFace(faces[2]);
                new_faces[4]->AddFace(faces[3]);

                //AGGIORNO VICINI g, e, i, f
                for(int j=0; j<2; j++)
                {
                    meshPointer->Edge(edges[j]->Id())->AddCell(cells[j]);
                    meshPointer->Edge(edges[j +2]->Id())->AddCell(cells[j]);
                }

                //AGGIORNO VICINI DI A, B, D, E
                for(int j=0; j<4; j++)
                {
                    new_faces[j]->AddFace(new_faces[4]);
                    new_faces[j]->AddCell(cells[j%2]);
                }

                //AGGIORNO VICINI C, F
                for(int j=2; j<4; j++)
                {
                     meshPointer->Face(faces[j]->Id())->AddFace(new_faces[j]);
                     meshPointer->Face(faces[j]->Id())->AddFace(new_faces[j-2]);
                     meshPointer->Face(faces[j]->Id())->AddFace(new_faces[4]);
                     meshPointer->Face(faces[j]->Id())->AddCell(cells[j-2]);
                }
                unsigned int id_0 = cells[0]->Id();
                unsigned int id_1 = cells[1]->Id();
                if(CellIntegrityCheck(id_0) == Output::Success)
                    cout<<"Cell integrity ID "<< id_0 << " correct\n";
                if(CellIntegrityCheck(id_1) == Output::Success)
                    cout<<"Cell integrity ID "<< id_1 << " correct\n";
            }

        }
        return Output::Success;

    }

    const Output::ExitCodes RefinerTetra::RefineMesh()
    {

        for(int i = 0; i < idCellToRefine.size(); i++)
        {
            if(CutTetra(*meshPointer->Cell(idCellToRefine[i])) == Output::Success)
            {
                cout << "\t\tOperazione di Refining eseguita sul tetra di id " << meshPointer->Cell(idCellToRefine[i])->Id() << endl;
            }
        }
        return Output::Success;
    }

    const GenericPoint& RefinerTetra::FirstCut(unsigned int idCell)
    {
        GenericEdge* MaxEdge = meshPointer->Edge(FindMaxEdge(*meshPointer->Cell(idCell))->Id());
        const Vector3d c_point = 0.5 * ( MaxEdge-> Point(0) ->Coordinates() + MaxEdge -> Point(1) ->Coordinates());
        //  CREAZIONE DEL PUNTO MEDIO
        const vector<Vector3d> x = {c_point};
        meshPointer->CutEdgeWithPoints(MaxEdge->Id(), x);
    }

    const Output::ExitCodes RefinerTetra::CellIntegrityCheck(const unsigned int& cell_id)
    {
        const GenericCell& cell = *meshPointer->Cell(cell_id);
        ///CONTROLLO I PUNTI
        if(cell.NumberOfPoints() != 4)//verifico il numero
            {cout<<"Error: Number of points is not 4\n"; return Output::GenericError;}

        ///CONTROLLO LATI
        if(cell.NumberOfEdges() != 6)
            {cout<<"Error: Number of edges is not 6\n"; return Output::GenericError;}
        for(int i=0; i<6; i++)
        {
            const GenericEdge* cell_edge = cell.Edge(i);
            for(int j=0; j<2; j++)//controllo della correttezza dei punti
            {
                const GenericPoint* edge_point = cell_edge->Point(j);
                if(edge_point != cell.Point(0) && edge_point != cell.Point(1) && edge_point != cell.Point(2) && edge_point != cell.Point(3))
                {
                    cout<<"Error: Edge with wrong points\n";
                    return Output::GenericError;
                }
            }
            unsigned int counter = 0;
            for(int j=0; j<cell_edge->NumberOfFaces(); j++)//controllo che almeno due faccie condividano lo stesso lato
            {
                const GenericFace* edge_face = cell_edge->Face(j);
                if(edge_face == cell.Face(0) || edge_face == cell.Face(1) || edge_face == cell.Face(2) || edge_face == cell.Face(3))
                    counter ++;
            }
            if(counter != 2)
            {
                cout<<"Error: Edge with " << counter << " of the faces instead of 2\n";
                return Output::GenericError;
            }
        }
        ///CONTROLLO FACCIE
        if(cell.NumberOfFaces() != 4)
            {cout<<"Error: Number of faces is not 4\n"; return Output::GenericError;}
        for(int i=0; i<4; i++)
        {
            const GenericFace* cell_face = cell.Face(i);
            if(cell_face->NumberOfEdges() != 3)
            {
                cout<<"Error: Face with wrong number of edges\n";
                return Output::GenericError;
            }
            int counter_edge =0;
            //CONTROLLO PRESENZA DEI LATI NEL TETRAEDRO
            for(int j=0; j<6; j++)
            {
                if(cell.Edge(j) == cell_face->Edge(0) || cell.Edge(j) == cell_face->Edge(1) || cell.Edge(j) == cell_face->Edge(2))
                    counter_edge++;
            }
            if(counter_edge != 3)
            {
                cout<<"Error: Face with " << counter_edge <<" in the cell\n";
                return Output::GenericError;
            }
            //CONTROLLO PRESENZA LATI DOPPI
            if(cell_face->Edge(0) == cell_face->Edge(1) || cell_face->Edge(0) == cell_face->Edge(2) || cell_face->Edge(1) == cell_face->Edge(2))
            {
                cout<<"Error: face with double edges\n";
                return Output::GenericError;
            }
            if(cell_face->NumberOfPoints() != 3)
            {
                cout<<"Error: Face with wrong number of points\n";
                return Output::GenericError;
            }

            for(int j=0; j<3; j++)//controllo della correttezza dei punti
            {
                const GenericPoint* face_point = cell_face->Point(j);
                if(face_point != cell.Point(0) && face_point != cell.Point(1) && face_point != cell.Point(2) && face_point != cell.Point(3))
                {
                    cout<<"Error: face with wrong points\n";
                    return Output::GenericError;
                }
            }
        }
        return Output::Success;
    }
}
