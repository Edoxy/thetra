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
        //ricerca lato più lungo
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

            if(cutter.CutCellTetra(cell, normal, translation, 1.0E-7) == Output::Success)
            {
                cout << "success" << endl;
            }

            RecoverConformity(cell, point, *edge_to_split);
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
            if(long_edge.Cell(i) != &cell)
            {
                //ciclo all'interno delle celle vicine di questo lato
                //creazione dei due nuovi lati
                vector <const GenericPoint*> points;
                vector <GenericEdge*> new_edges;
                vector <const GenericFace*> faces;
                vector <GenericFace*> new_faces;
                vector <GenericCell*> cells;

                const GenericCell& current_cell = *long_edge.Cell(i);

                for(int j = 0; j < long_edge.NumberOfFaces(); j++)
                {
                    const GenericFace& current_face = *long_edge.Face(j);

                    for(int k = 0; k < current_cell.NumberOfFaces(); k++)
                    {
                        const GenericFace& current_face_in_cell = *current_cell.Face(k);
                        //controllo se faccia già tagliata
                        if(&current_face == &current_face_in_cell  && !current_face.HasChilds())
                        {
                            faces.push_back(&current_face);
                            for(int x = 0; x < 3; x++)
                            {
                                if(current_face.Point(x) != long_edge.Point(0) && current_face.Point(x) != long_edge.Point(1))
                                {
                                    unsigned int pos_point = points.size();
                                    points.push_back(current_face.Point(x));

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
                                    new_faces[pos_new_faces]->SetFather(faces[pos_faces -1]);
                                    new_faces[pos_new_faces + 1]->SetFather(faces[pos_faces -1]);
                                    //aggiunta punti
                                    new_faces[pos_new_faces]->AddPoint(&middlePoint);
                                    new_faces[pos_new_faces]->AddPoint(points[pos_point]);
                                    new_faces[pos_new_faces]->AddPoint(long_edge.Point(0));
                                    new_faces[pos_new_faces +1]->AddPoint(&middlePoint);
                                    new_faces[pos_new_faces +1]->AddPoint(points[pos_point]);
                                    new_faces[pos_new_faces +1]->AddPoint(long_edge.Point(1));

                                    //aggiunta dei vicini
                                    new_faces[pos_new_faces]->AddEdge(a);
                                    new_faces[pos_new_faces]->AddEdge(new_edges[pos_edge]);
                                    new_faces[pos_new_faces +1]->AddEdge(b);
                                    new_faces[pos_new_faces +1]->AddEdge(new_edges[pos_edge]);
                                    for(int y = 0; y < 3; y++)
                                    {
                                        if(faces[pos_faces]->Edge(y) != &long_edge && (faces[pos_faces]->Edge(y)->Point(0) == long_edge.Point(0) || faces[pos_faces]->Edge(y)->Point(1) == long_edge.Point(0)))
                                        {
                                            new_faces[pos_new_faces]->AddEdge(faces[pos_faces]->Edge(y));
                                        }else if(faces[pos_faces]->Edge(y) != &long_edge)
                                        {
                                            new_faces[pos_new_faces +1]->AddEdge(faces[pos_faces]->Edge(y));
                                        }
                                    }

                                    meshPointer->AddFace(new_faces[pos_new_faces]);
                                    meshPointer->AddFace(new_faces[pos_new_faces +1]);
                                }
                            }
                        }
                        else if(&current_face == &current_face_in_cell)
                        {
                            //aggiunta faccie già create
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
                            //aggiunta lato già creato e del punto
                            for(int x = 0; x < 3; x++)
                            {
                                if(current_face.Point(x) != long_edge.Point(0) && current_face.Point(x) != long_edge.Point(1))
                                {
                                    unsigned int pos_point = points.size();
                                    points.push_back(current_face.Point(x));

                                    for (int y = 0; y < 3; y++)
                                    {
                                        unsigned int pos_face = new_faces.size() -2;
                                        if (new_faces[pos_face]->Edge(y)->Point(0) == &middlePoint || new_faces[pos_face]->Edge(y)->Point(1) == &middlePoint)
                                        {
                                            if(new_faces[pos_face]->Edge(y)->Point(0) == points[pos_point] || new_faces[pos_face]->Edge(y)->Point(1) == points[pos_point])
                                            {
                                                GenericEdge* e = meshPointer->Edge(new_faces[pos_face]->Edge(y)->Id());
                                                new_edges.push_back(e);
                                            }
                                        }
                                    }
                                }

                            }

                        }
                    }
                }
                for(int j=0; j<2; j++)
                {
                    //Creazione nuovi tetraedri
                    cells.push_back(meshPointer->CreateCell());
                    cells[j]->AddPoint(points[0]);
                    cells[j]->AddPoint(points[1]);
                    cells[j]->AddPoint(long_edge.Point(j));
                    cells[j]->AddPoint(&middlePoint);
                    meshPointer->AddCell(cells[j]);
                }
                meshPointer->Cell(current_cell.Id())->SetState(false);
            }

        }

        return Output::Success;
    }

    const Output::ExitCodes RefinerTetra::RefineMesh()
    {

        for(int i = 0; i < idCellToRefine.size(); i++)
        {
            cout << meshPointer->Cell(idCellToRefine[i])->Id() << endl;
            CutTetra(*meshPointer->Cell(i));
        }
        return Output::Success;
    }
}
