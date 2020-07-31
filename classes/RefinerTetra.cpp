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
        //Controllo che non sia gi� stato tagliato
        if(cell.IsActive())
        {
            //cerco il lato da tagliare
            const GenericEdge* edge_to_split = this -> FindMaxEdge(cell);
            //trovo il punto medio
            Vector3d point = 0.5 * (edge_to_split -> Point(0) ->Coordinates() + edge_to_split -> Point(1) ->Coordinates());


            CutterMesh3D cutter;
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
            }

            if(RecoverConformity(cell, point, *edge_to_split) == Output::Success)
            {
                cout << "Recover Conformity successful"  << endl;
            }
        }
        return Output::Success;
    }

    const Output::ExitCodes RefinerTetra::RecoverConformity(GenericCell& cell, const Vector3d new_point, const GenericEdge& long_edge)
    {


        const GenericPoint& middlePoint = *meshPointer->Point(meshPointer->NumberOfPoints() - 1);

        //trovo a e b
        GenericEdge* a;
        GenericEdge* b;
        if(static_cast<const GenericEdge*>(long_edge.Child(0))->Point(0) == long_edge.Point(0) || static_cast<const GenericEdge*>(long_edge.Child(0))->Point(1) == long_edge.Point(0))
        {
            a = meshPointer->Edge(long_edge.Child(0)->Id());
            b = meshPointer->Edge(long_edge.Child(1)->Id());
        }else
        {
            b = meshPointer->Edge(long_edge.Child(0)->Id());
            a = meshPointer->Edge(long_edge.Child(1)->Id());
        }

        for (int i = 0; i < long_edge.NumberOfCells(); i++)
        {
            if(long_edge.Cell(i) != &cell && !long_edge.Cell(i)->HasChilds())
            {

                //ciclo all'interno delle celle vicine di questo lato
                //creazione dei due nuovi lati
                vector <const GenericPoint*> points;
                vector <const GenericEdge*> edges;
                vector <GenericEdge*> new_edges;
                vector <const GenericFace*> faces;
                vector <GenericFace*> new_faces;
                vector <GenericCell*> cells;
                const GenericCell& current_cell = *long_edge.Cell(i);
                cout << "   Nuova cella da rifinire" << endl;
                for(int j = 0; j < long_edge.NumberOfFaces(); j++)
                {
                    const GenericFace& current_face = *long_edge.Face(j);

                    for(int k = 0; k < current_cell.NumberOfFaces(); k++)
                    {
                        const GenericFace& current_face_in_cell = *current_cell.Face(k);
                        //controllo se faccia gi� tagliata
                        if(&current_face == &current_face_in_cell  && current_face.NumberOfChilds() == 1)
                        {
                            cout << "faccia da tagliare" << endl;
                            GenericFace& current_child = *meshPointer->Face(current_face.Child(0)->Id());
                            faces.push_back(&current_child);
                            for(int x = 0; x < 4; x++)
                            {
                                if(current_child.Point(x) != long_edge.Point(0) && current_child.Point(x) != long_edge.Point(1) && current_child.Point(x) != &middlePoint)
                                {
                                    unsigned int pos_point = points.size();
                                    points.push_back(current_child.Point(x));

                                    //creazione nuovo edge
                                    unsigned int pos_edge = new_edges.size();
                                    new_edges.push_back(meshPointer->CreateEdge());
                                    new_edges[pos_edge]->AddPoint(&middlePoint);
                                    new_edges[pos_edge]->AddPoint(points[pos_point]);
                                    meshPointer->AddEdge(new_edges[pos_edge]);

                                    //Creazione nuove facce
                                    unsigned int pos_new_faces = new_faces.size();
                                    unsigned int pos_faces = faces.size() -1;
                                    new_faces.push_back(meshPointer->CreateFace());
                                    new_faces.push_back(meshPointer->CreateFace());

                                    //deattivazione faccia
                                    meshPointer->Face(faces[pos_faces]->Id())->SetState(false);
                                    meshPointer->Face(faces[pos_faces]->Id())->AddChild(new_faces[pos_new_faces]);
                                    meshPointer->Face(faces[pos_faces]->Id())->AddChild(new_faces[pos_new_faces +1]);

                                    //settaggio padre
                                    new_faces[pos_new_faces]->SetFather(faces[pos_faces]);
                                    new_faces[pos_new_faces + 1]->SetFather(faces[pos_faces]);

                                    //AGGIUNTA PUNTI ALLE FACCE
                                    new_faces[pos_new_faces]->AddPoint(&middlePoint);
                                    new_faces[pos_new_faces]->AddPoint(points[pos_point]);
                                    new_faces[pos_new_faces]->AddPoint(long_edge.Point(0));
                                    new_faces[pos_new_faces +1]->AddPoint(&middlePoint);
                                    new_faces[pos_new_faces +1]->AddPoint(points[pos_point]);
                                    new_faces[pos_new_faces +1]->AddPoint(long_edge.Point(1));

                                    //AGGIUNTA LATI ALLE FACCE
                                    new_faces[pos_new_faces]->AddEdge(a);
                                    new_faces[pos_new_faces]->AddEdge(new_edges[pos_edge]);
                                    new_faces[pos_new_faces +1]->AddEdge(b);
                                    new_faces[pos_new_faces +1]->AddEdge(new_edges[pos_edge]);

                                    GenericEdge* g;
                                    GenericEdge* e;
                                    for(int y = 0; y < 4; y++)
                                    {
                                        if(faces[pos_faces]->Edge(y) != a && faces[pos_faces]->Edge(y) != b  && (faces[pos_faces]->Edge(y)->Point(0) == long_edge.Point(0) || faces[pos_faces]->Edge(y)->Point(1) == long_edge.Point(0)))
                                        {
                                            new_faces[pos_new_faces]->AddEdge(faces[pos_faces]->Edge(y));
                                            g = meshPointer->Edge(faces[pos_faces]->Edge(y)->Id());
                                        }else if(faces[pos_faces]->Edge(y) != a && faces[pos_faces]->Edge(y) != b)
                                        {
                                            new_faces[pos_new_faces +1]->AddEdge(faces[pos_faces]->Edge(y));
                                            e = meshPointer->Edge(faces[pos_faces]->Edge(y)->Id());
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
                                    //AGGIORNO g, e
                                    unsigned int x = edges.size()-1;

                                    meshPointer->Edge(edges[x-1]->Id())->AddFace(new_faces[pos_new_faces]);
                                    meshPointer->Edge(edges[x]->Id())->AddFace(new_faces[pos_new_faces +1]);
                                    cout << "id " << edges[x-1]->Id() << " " << edges[x]->Id() << endl;
                                    cout << edges << endl;
                                }
                            }
                        }
                        //CASO IN CUI LA FACCIA SIA GIA' STATA TAGLIATA
                        else if(&current_face == &current_face_in_cell && current_face.NumberOfChilds() == 2)
                        {
                            //aggiunta facce già create

                            cout << "Faccia già tagliata" << endl;

                            unsigned int pos_faces = faces.size();
                            unsigned int pos_new_faces = new_faces.size();
                            faces.push_back(&current_face);
                            if(meshPointer->Face(current_face.Child(0)->Id())->Edge(0) == a || meshPointer->Face(current_face.Child(0)->Id())->Edge(1) == a || meshPointer->Face(current_face.Child(0)->Id())->Edge(2) == a)
                            {
                                new_faces.push_back(meshPointer->Face(current_face.Child(0)->Id()));
                                new_faces.push_back(meshPointer->Face(current_face.Child(1)->Id()));
                            }else
                            {
                                new_faces.push_back(meshPointer->Face(current_face.Child(1)->Id()));
                                new_faces.push_back(meshPointer->Face(current_face.Child(0)->Id()));
                            }
                            //aggiunta lato già creato e del punto opposto al medio;  ricerca g ed e
                            GenericEdge* temp_a;
                            GenericEdge* temp_b;
                            for(int x = 0; x < 3; x++)
                            {
                                if(current_face.Point(x) != long_edge.Point(0) && current_face.Point(x) != long_edge.Point(1))
                                {
                                    unsigned int pos_point = points.size();
                                    points.push_back(current_face.Point(x));

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
                                }
                                //ricerca di g, e
                                if(faces[pos_faces]->Edge(x) != &long_edge && (faces[pos_faces]->Edge(x)->Point(0) == long_edge.Point(0) || faces[pos_faces]->Edge(x)->Point(1) == long_edge.Point(0)))
                                {
                                    temp_a = meshPointer->Edge(faces[pos_faces]->Edge(x)->Id());
                                }else if(faces[pos_faces]->Edge(x) != &long_edge)
                                {
                                    temp_b = meshPointer->Edge(faces[pos_faces]->Edge(x)->Id());
                                }
                            }
                            edges.push_back(temp_a);
                            edges.push_back(temp_b);

                        }
                    }
                }
                //cout << "id" << edges[0]->Id() << " " << edges[1]->Id() << " " << edges[2]->Id() << " "  << edges[3]->Id() << endl;
                //TROVO H
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
                //Cerco Facce C e F
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

                //AGGIUNTA VICINI E e D (vertici)   (Faccia G?)/lati c e d?
                for(int j =0; j<2; j++)
                {
                    meshPointer->Point(points[j]->Id())->AddCell(cells[0]);
                    meshPointer->Point(points[j]->Id())->AddCell(cells[1]);
                }

                //AGGIUNTA VICINI MIDDELPOINT    (Faccia G?)/lati c e d?
                meshPointer->Point(middlePoint.Id())->AddCell(cells[0]);
                meshPointer->Point(middlePoint.Id())->AddCell(cells[1]);
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
            }

        }
        return Output::Success;

    }

    const Output::ExitCodes RefinerTetra::RefineMesh()
    {

        for(int i = 0; i < idCellToRefine.size(); i++)
        {
            if(CutTetra(*meshPointer->Cell(i)) == Output::Success)
            {
                cout << "\n Operazione di Refining eseguita sul tetra di id " << meshPointer->Cell(idCellToRefine[i])->Id() << endl;
            }
        }
        return Output::Success;
    }
}
