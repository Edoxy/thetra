#include "RefinerTetra.hpp"
#include "Output.hpp"
#define DEBUG 0 //0: None; 1: few; 2:All check; 3: Vector prints
#define ASP_TOLL 4
#define DIM_TOLL 0.4
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
        //ricerca lato piu' lungo
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

    const Output::ExitCodes RefinerTetra::Cutter(GenericCell& cell)
    {
        //CONTROLLO CHE LA CELLA SIA ATTIVA
        if(cell.IsActive())
        {

            //RICERCA DEL LATO DA TAGLIARE
            GenericEdge& MaxEdge = *meshPointer->Edge(FindMaxEdge(*meshPointer->Cell(cell.Id()))->Id());
            const Vector3d c_point = 0.5 * ( MaxEdge.Point(0) ->Coordinates() + MaxEdge.Point(1) ->Coordinates());

            //CREAZIONE DEL PUNTO MEDIO
            GenericPoint& middlePoint = *meshPointer->CreatePoint();
            middlePoint.SetCoordinates(c_point);
            meshPointer->AddPoint(&middlePoint);

            //CREAZIONE FIGLI
            vector <GenericEdge*> a_b;
            MaxEdge.SetState(false);
            for(int i = 0; i<2; i++)
            {
                a_b.push_back(meshPointer->CreateEdge());
                a_b[i]->SetFather(&MaxEdge);
                a_b[i]->AddPoint(MaxEdge.Point(i));
                a_b[i]->AddPoint(&middlePoint);
                MaxEdge.AddChild(a_b[i]);
                meshPointer->AddEdge(a_b[i]);
            }

            //INIZIO DEL TAGLIO DEI TETRAEDRI INTORNO A MAXEDGE
            vector <unsigned int> bad_cell;//vettore con gli id delle celle da tagliare per pessima qualit??


            //CICLO NELLE CELLE CON IL LATO CHE E' STATO TAGLIATO
            for (int i = 0; i < MaxEdge.NumberOfCells(); i++)
            {
                //CREAZIONE VETTORI PER MEMORIZZAZIONE TETRAEDRO
                vector <const GenericPoint*> points;
                vector <const GenericEdge*> edges;
                vector <GenericEdge*> new_edges;
                vector <const GenericFace*> faces;
                vector <GenericFace*> new_faces;
                vector <GenericCell*> cells;

                const GenericCell& current_cell = *MaxEdge.Cell(i);
                if(current_cell.IsActive())
                {
#if DEBUG > 0
                    cout << "Nuova cella da rifinire\tID\t" << current_cell.Id() << endl;
#endif
                    if(CellIntegrityCheck(current_cell.Id()) == Output::Success)
                    {
#if DEBUG > 1
                        cout<<"Cell Integrity correct\n";
#endif
                    }else
                    {
                        cout<<"ERROR: Cell ID "<< current_cell.Id() << " NOT CORRECT\n";
                    }
                    unsigned int counter = 0;//controlla il numero di facce verificate(devono essere due)
                    //CICLO NELLE FACCE VICINE AL LATO TAGLIATO
                    for(int j = 0; j < 4; j++)
                    {
                        const GenericFace& current_face = *current_cell.Face(j);
                        //CICLO NELLE FACCE DEL TETRAEDRO CORRENTE
                        if(current_face.Edge(0) == &MaxEdge || current_face.Edge(1) == &MaxEdge || current_face.Edge(2) == &MaxEdge)
                        {
                            //CONTROLLO FACCIA SE GIA' TAGLIATA E SE E' UNA DELLE DUE CHE CI INTERESSANO
                            if(!current_face.HasChilds())
                            {
                                counter++;
#if DEBUG > 1
                                cout << "\tFaccia di id " << current_face.Id() << " da tagliare numero " << counter << "\n";
#endif
                                unsigned int pos_faces = faces.size();  //INDICE POSIZIONE NEL VETTORE faces DELLA FACCIA CHE STIAMO TAGLIANDO
                                faces.push_back(meshPointer->Face(current_face.Id()));

                                //RICERCA DEL PUNTO OPPOSTO A MIDDLEPOINT (E, D)
                                unsigned int pos_point = points.size();  //INDICE POSIZIONE NEL VETTORE POINTS DEL PUNTO OPPOSTO A MIDDLE POINT
                                for(int x = 0; x < 3; x++)
                                {
                                    if(faces[pos_faces]->Point(x) != MaxEdge.Point(0) && faces[pos_faces]->Point(x) != MaxEdge.Point(1) && faces[pos_faces]->Point(x) != &middlePoint)
                                    {
                                        points.push_back(faces[pos_faces]->Point(x));
                                    }
                                }
                                //CREAZIONE DEL NUOVO EDGE
                                unsigned int pos_new_edge = new_edges.size(); //INDICE POSIZIONE NEL VETTORE new_edges DEL NUOVO LATO CREATO
                                new_edges.push_back(meshPointer->CreateEdge());
                                new_edges[pos_new_edge]->AddPoint(&middlePoint);
                                new_edges[pos_new_edge]->AddPoint(points[pos_point]);
                                meshPointer->AddEdge(new_edges[pos_new_edge]);

                                //CREAZIONE DELLE DUE NUOVE FACCE
                                unsigned int pos_new_faces = new_faces.size(); //INDICE POSIZIONE NEL VETTORE new_faces DELLA PRIMA DELLE FACCE CREATE

                                for(int x = 0; x < 2; x++)
                                {
                                    //CREAZIONE FACCIA E AGGIUNTA ALLA MESH
                                    new_faces.push_back(meshPointer->CreateFace());
                                    meshPointer->AddFace(new_faces[pos_new_faces +x]);
                                    //SETTAGIO DELLA FACCIA PADRE
                                    new_faces[pos_new_faces +x]->SetFather(faces[pos_faces]);
                                    //AGGIUNTA DEI PUNTI NELLA FACCIA
                                    new_faces[pos_new_faces +x]->InitializePoints(3);
                                    new_faces[pos_new_faces +x]->AddPoint(&middlePoint);
                                    new_faces[pos_new_faces +x]->AddPoint(points[pos_point]);
                                    new_faces[pos_new_faces +x]->AddPoint(MaxEdge.Point(x));
                                }

                                //DEATTIVAZIONE DELLA FACCIA PADRE E AGGIUNTA DEI FIGLI
                                meshPointer->Face(faces[pos_faces]->Id())->SetState(false);
                                meshPointer->Face(faces[pos_faces]->Id())->InitializeChilds(2);
                                meshPointer->Face(faces[pos_faces]->Id())->AddChild(new_faces[pos_new_faces]);
                                meshPointer->Face(faces[pos_faces]->Id())->AddChild(new_faces[pos_new_faces +1]);

                                //RICERCA g, e
                                GenericEdge* g;
                                GenericEdge* e;
                                unsigned int pos_edges = edges.size();
                                for(int y = 0; y < 3; y++)
                                {
                                    if(faces[pos_faces]->Edge(y) != &MaxEdge)
                                    {
                                        if(faces[pos_faces]->Edge(y)->Point(0) == MaxEdge.Point(0) || faces[pos_faces]->Edge(y)->Point(1) == MaxEdge.Point(0))
                                        {
                                            g = meshPointer->Edge(faces[pos_faces]->Edge(y)->Id());
                                        }else
                                        {
                                            e = meshPointer->Edge(faces[pos_faces]->Edge(y)->Id());
                                        }
                                    }
                                }
                                edges.push_back(g);
                                edges.push_back(e);

                                //AGGIUNTA LATI ALLE FACCE
                                for(int x = 0; x < 2; x++)
                                {
                                    new_faces[pos_new_faces +x]->InitializeEdges(3);
                                    new_faces[pos_new_faces +x]->AddEdge(a_b[x]);
                                    new_faces[pos_new_faces +x]->AddEdge(new_edges[pos_new_edge]);
                                    new_faces[pos_new_faces +x]->AddEdge(edges[pos_edges + x]);
                                }

                                //AGGIORNO VICINI c E d
                                new_edges[pos_new_edge]->AddFace(new_faces[pos_new_faces]);
                                new_edges[pos_new_edge]->AddFace(new_faces[pos_new_faces +1]);

                                //AGGIORNO a E b
                                for(int x = 0; x < 2; x++)
                                {
                                    a_b[x]->AddFace(new_faces[pos_new_faces + x]);
                                    a_b[x]->AddEdge(new_edges[pos_new_edge]);
                                }

                                //AGGIORNO VICNI NUOVE FACCE
                                new_faces[pos_new_faces]->AddFace(new_faces[pos_new_faces +1]);
                                new_faces[pos_new_faces +1]->AddFace(new_faces[pos_new_faces]);

                                //AGGIORNO g, e oppure i, f
                                for(int x = 0; x<2; x++)
                                    meshPointer->Edge(edges[pos_edges +x]->Id())->AddFace(new_faces[pos_new_faces +x]);
                            }
                            ///CASO IN CUI LA FACCIA SIA GIA' STATA TAGLIATA
                            else
                            {
                                counter++;
                                //AGGIUNTA FACCIA SE GIA' TAGLIATA
#if DEBUG > 1
                                cout << "\tFaccia di id " << current_face.Id() << " gia' tagliata numero " << counter << "\n" << endl;
#endif
                                unsigned int pos_faces = faces.size();//indice posizione vettore faces
                                unsigned int pos_new_faces = new_faces.size();//indice posizione vettore new_faces
                                faces.push_back(&current_face);
                                if(current_face.NumberOfChilds() != 2)
                                    cout << "ERROR: face with "<<current_face.NumberOfChilds()<<" childs\n";

                                if(meshPointer->Face(current_face.Child(0)->Id())->Edge(0) == a_b[0] || meshPointer->Face(current_face.Child(0)->Id())->Edge(1) == a_b[0] || meshPointer->Face(current_face.Child(0)->Id())->Edge(2) == a_b[0])
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
                                    if(new_faces[pos_new_faces]->Point(x) != MaxEdge.Point(0) && new_faces[pos_new_faces]->Point(x) != &middlePoint)
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
                                    if(new_faces[pos_new_faces]->Edge(y) != a_b[0] && new_faces[pos_new_faces]->Edge(y) != new_edges[pos_new_edges])
                                    {
                                        edges.push_back(meshPointer->Edge(new_faces[pos_new_faces]->Edge(y)->Id()));
                                        count_ge++;
                                    }
                                }
                                if(count_ge != 1)
                                    cout << "ERROR: too many g edges\n";
                                count_ge = 0;

                                for(int y = 0; y < 3; y++)//RICERCA DI e
                                {
                                    if(new_faces[pos_new_faces +1]->Edge(y) != a_b[1] && new_faces[pos_new_faces +1]->Edge(y) != new_edges[pos_new_edges])
                                    {
                                        edges.push_back(meshPointer->Edge(new_faces[pos_new_faces +1]->Edge(y)->Id()));
                                        count_ge++;
                                    }
                                }
                                if(count_ge != 1)
                                    cout << "ERROR: too many e edges\n";
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
                    //RICERCA DELLE FACCE C e F
                    for(int j = 0; j < 4; j++)
                    {
                        faces.resize(4);
                        if(current_cell.Face(j)->Edge(0) == edges[4] || current_cell.Face(j)->Edge(1) == edges[4] || current_cell.Face(j)->Edge(2) == edges[4])
                        {
                            if(current_cell.Face(j)->Point(0) == MaxEdge.Point(0) || current_cell.Face(j)->Point(1) == MaxEdge.Point(0) || current_cell.Face(j)->Point(2) == MaxEdge.Point(0))
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

#if DEBUG > 2

                    cout << "Tabella Nomi \n";
                    cout << points[0]->Id() << "\t" << new_edges[0]->Id() << "\t" << new_faces[0]->Id() << "\t" << edges[0]->Id() << "\t" << faces[0]->Id() << "\t" << a_b[0] ->Id() << "\n";
                    cout << points[1]->Id() << "\t" << new_edges[1]->Id() << "\t" << new_faces[1]->Id() << "\t" << edges[1]->Id() << "\t" << faces[1]->Id() << "\t" << a_b[1] ->Id() << "\n";
                    cout << "\t\t"<< new_faces[2]->Id() << "\t" << edges[2]->Id() << "\t" << faces[2]->Id() << "\n";
                    cout << "\t\t"<< new_faces[3]->Id() << "\t" << edges[3]->Id() << "\t" << faces[3]->Id() << "\n";
                    cout << "\t\t"<< new_faces[4]->Id() << "\t" << edges[4]->Id() << "\t" <<"\n";

#endif
                    //CREAZIONE NUOVE CELLE
                    for(int j=0; j<2; j++)
                    {
                        //Creazione nuovi tetraedri
                        cells.push_back(meshPointer->CreateCell());
                        //aggiunta punti
                        cells[j]->AddPoint(points[0]);
                        cells[j]->AddPoint(points[1]);
                        cells[j]->AddPoint(MaxEdge.Point(j));
                        cells[j]->AddPoint(&middlePoint);
                        //aggiunta edges
                        cells[j]->AddEdge(new_edges[0]);
                        cells[j]->AddEdge(new_edges[1]);
                        cells[j]->AddEdge(edges[4]);
                        cells[j]->AddEdge(edges[j]);
                        cells[j]->AddEdge(edges[j+2]);
                        cells[j]->AddEdge(a_b[j]);
                        //aggiunta facce
                        cells[j]->AddFace(new_faces[4]);
                        cells[j]->AddFace(new_faces[j]);
                        cells[j]->AddFace(new_faces[j+2]);
                        cells[j]->AddFace(faces[j+2]);

                        meshPointer->AddCell(cells[j]);
                        meshPointer->Cell(current_cell.Id())->AddChild(cells[j]);
                        cells[j]->SetFather(&current_cell);
                    }
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
                    a_b[0]->AddCell(cells[0]);
                    a_b[1]->AddCell(cells[1]);


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

                    ///CONTROLLO INTEGRITA' DEI FIGLI CREATI
                    unsigned int id_0 = cells[0]->Id();
                    unsigned int id_1 = cells[1]->Id();
                    if(CellIntegrityCheck(id_0) == Output::Success)
                    {
#if DEBUG > 1
                        cout<<"Cell integrity ID "<< id_0 << " correct\n";
#endif
                    }else
                    {
                        cout<<"ERROR: Cell ID " << id_0 << " NOT CORRECT\n";
                    }

                    if(CellIntegrityCheck(id_1) == Output::Success)
                    {
#if DEBUG > 1
                        cout<<"Cell integrity ID "<< id_1 << " correct\n";
#endif
                    }else
                    {
                        cout<<"ERROR: Cell ID " << id_1 << " NOT CORRECT\n";
                    }

                    ///CONTROLLO INTEGRIT?? DEI FIGLI CREATI
                    if(CellQuality(id_0) > ASP_TOLL)
                    {
                        bad_cell.push_back(id_0);
                    }
                    if(CellQuality(id_1) > ASP_TOLL)
                    {
                        bad_cell.push_back(id_1);
                    }
                }
            }
            for(int i = 0; i< bad_cell.size(); i++)
            {
                Cutter(*meshPointer->Cell(bad_cell[i]));
            }

        }
        return Output::Success;
    }

    const Output::ExitCodes RefinerTetra::RefineMesh()
    {

#if DEBUG > 2
        cout << idCellToRefine << endl;
#endif
        for(int i = 0; i < idCellToRefine.size(); i++)
        {
#if DEBUG > 2
            EdgesCheck();
#endif
#if DEBUG > 1
            for(int j=0; j < meshPointer->NumberOfCells();j++)
            {
                if(CellIntegrityCheck(j) == Output::Success)
                    cout<<"CELL integrity ID "<< j << " correct\t" << meshPointer->Cell(j);
                else
                {
                    cout << "CHECK ERROR: Cell Integrity; ID " << j << endl;
                }
                cout << "\tIndice AspectRatio " << CellQuality(j) << endl;
            }
#endif
#if DEBUG > 1
            cout << "Inizio Refining cella id " << idCellToRefine[i]<< "\tIndirizzo "<< meshPointer->Cell(idCellToRefine[i]) << endl;
#endif
            if(Cutter(*meshPointer->Cell(idCellToRefine[i])) == Output::Success)
            {
#if DEBUG > 0
                cout << "\tOperazione di Refining eseguita sul tetra di id " << meshPointer->Cell(idCellToRefine[i])->Id() << endl;
#endif
            }else
            {
                cout << "ERROR: Operazione refinig fallita; ID " << idCellToRefine[i] << endl;
            }
        }
        return Output::Success;
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
                    cout<<"Error: Edge with wrong points di id " << cell_edge->Id() << "\n";
                    return Output::GenericError;
                }
            }
            unsigned int counter = 0;
            for(int j=0; j<cell_edge->NumberOfFaces(); j++)//controllo che due faccie condividano lo stesso lato
            {
                const GenericFace* edge_face = cell_edge->Face(j);
                if(edge_face == cell.Face(0) || edge_face == cell.Face(1) || edge_face == cell.Face(2) || edge_face == cell.Face(3))
                    counter ++;
            }
            if(counter != 2)
            {
                cout<<"Error: Edge di id " << cell_edge->Id() << " with " << counter << " of the faces instead of 2\n";
                return Output::GenericError;
            }
            for(int j=0; j<cell_edge->NumberOfCells();j++)//controllo assenza di vicini vuoti
            {
                if(cell_edge->Cell(j) == 0 || cell_edge->Cell(j) == NULL)
                {
                    cout << "ERROR: Edge di id " << cell_edge->Id() << " with a NULL neighbour\n";

                    return Output::GenericError;
                }
            }
        }
        ///CONTROLLO FACCE
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
            //CONTROLLO CHE LE FACCE TRA LORO ABBIANO UN SOLO LATO IN COMUNE
            if(i < 3)
            {
                for(int j=i+1; j<4; j++)
                {
                    int counter = 0;
                    for(int x=0; x<3; x++)
                    {
                        for(int y=0; y<3; y++)
                        {
                            if(cell_face->Edge(x) == cell.Face(j)->Edge(y))
                                counter++;
                        }
                    }
                    if(counter != 1)
                    {
                        cout<<"Error: 2 faces have " << counter << " in common\n";
                        return Output::GenericError;
                    }
                }
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

    const Output::ExitCodes RefinerTetra::EdgesCheck()
    {
        cout << "EDGE CHECK..." << endl;
        for(int i = 0; i < meshPointer->NumberOfEdges(); i++)
        {
            cout <<"Edge ID " << meshPointer->Edge(i)->Id() << endl;
            for(int j = 0; j<meshPointer->Edge(i)->NumberOfCells(); j++)
            {
                if(meshPointer->Edge(i)->Cell(j) == 0 || meshPointer->Edge(i)->Cell(j) == NULL)
                {
                    cout << "NULL" <<"\t";
                }

                cout << meshPointer->Edge(i)->Cell(j) << "\t";
            }
            cout << endl << endl;
        }
        cout << "END OF EDGE CHECK"<< endl;
    }

    long double RefinerTetra::CellQuality(const unsigned int cell_id)
    {
        const GenericCell& cell = *meshPointer->Cell(cell_id);
        const long double sqrt6 = 2.44948974;
        vector <Vector3d> alpha;
        vector <long double> N_norm;
        alpha.reserve(3);
        N_norm.reserve(4);
        for(int i = 0; i<6; i++)
        {
            if(cell.Edge(i)->Point(0) == cell.Point(0))
            {
                alpha.push_back(cell.Edge(i)->Point(1)->Coordinates() - cell.Edge(i)->Point(0)->Coordinates());
            }else if(cell.Edge(i)->Point(1) == cell.Point(0))
            {
                alpha.push_back(cell.Edge(i)->Point(0)->Coordinates() - cell.Edge(i)->Point(1)->Coordinates());
            }
        }
        for(int i = 0; i<4 ;i ++)
        {
            vector <Vector3d> face;
            face.reserve(2);
            for(int j = 0; j<3; j++)
            {
                if(cell.Face(i)->Edge(j)->Point(0) == cell.Face(i)->Point(0))
                {
                    face.push_back(cell.Face(i)->Edge(j)->Point(1)->Coordinates() -cell.Face(i)->Edge(j)->Point(0)->Coordinates());
                }else if(cell.Face(i)->Edge(j)->Point(1) == cell.Face(i)->Point(0))
                {
                    face.push_back(cell.Face(i)->Edge(j)->Point(0)->Coordinates() -cell.Face(i)->Edge(j)->Point(1)->Coordinates());
                }
            }
            N_norm.push_back(face[0].cross(face[1]).norm());
        }
        long double a = alpha[0].dot(alpha[1].cross(alpha[2]));
        const GenericEdge* max_edge = FindMaxEdge(*meshPointer->Cell(cell_id));
        long double h = (max_edge->Point(1)->Coordinates() -max_edge->Point(0)->Coordinates()).norm();
        if(h < DIM_TOLL)
        {
            return ASP_TOLL;
        }
        long double r = a/(N_norm[0] + N_norm[1] +N_norm[2] +N_norm[3]);
#if DEBUG > 2
        cout << "CHECK VECTOR ASPECT-RATIO\n" << "Alpha\t" << alpha <<endl;
        cout<<"N_norm\t" << N_norm<<endl;
        cout<<a<<endl<<h<<endl<<r<<endl<<"CHECK END...\n";
#endif
        double q =h/(2*sqrt6*r);
        if(q > 0)
        {
            return q;
        }else
        {
            return -q;
        }
    }
}
