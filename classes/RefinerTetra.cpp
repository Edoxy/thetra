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
        for (int i = 0; i < long_edge.NumberOfCells(); i++)
        {
            if(long_edge.Cell(i) != &cell)
            {
                //ciclo all'interno delle celle vicine di questo lato
                //creazione dei due nuovi lati
                vector <const GenericPoint*> points;
                vector <GenericEdge*> edges;
                vector <const GenericFace*> faces;
                vector <GenericFace*> new_faces;
                vector <GenericCell*> cells;

                const GenericCell& current_cell = *long_edge.Cell(i);

                for(int j = 0; j < long_edge.NumberOfFaces(); j++)
                {
                    if( !current_cell.Face(j)->HasChilds())
                    {
                        const GenericFace& current_face = *long_edge.Face(j);

                        for(int k = 0; k < current_cell.NumberOfFaces(); k++)
                        {
                            const GenericFace& current_face_in_cell = *current_cell.Face(k);
                            if(&current_face == &current_face_in_cell)
                            {
                                faces.push_back(&current_face);
                                for(int x = 0; x < 3; x++)
                                {
                                    if(current_face.Point(x) != long_edge.Point(0) && current_face.Point(x) != long_edge.Point(1))
                                        points.push_back(current_face.Point(x));

                                }
                            }

                        }
                    }
                }

                for(int j=0; j<2; j++)
                {
                    edges.push_back(meshPointer->CreateEdge());
                    edges[j]->AddPoint(&middlePoint);
                    edges[j]->AddPoint(points[j]);
                    meshPointer->AddEdge(edges[j]);

                    //facce nuove
                    new_faces.push_back(meshPointer->CreateFace());
                    new_faces.push_back(meshPointer->CreateFace());
                    int pos_faces = 2*j;
                    new_faces[pos_faces]->SetFather(faces[j]);
                    new_faces[pos_faces + 1]->SetFather(faces[j]);
                    new_faces[pos_faces]->AddPoint(&middlePoint);
                    new_faces[pos_faces]->AddPoint(points[j]);
                    new_faces[pos_faces]->AddPoint(long_edge.Point(0));
                    new_faces[pos_faces +1]->AddPoint(&middlePoint);
                    new_faces[pos_faces +1]->AddPoint(points[j]);
                    new_faces[pos_faces +1]->AddPoint(long_edge.Point(1));

                    meshPointer->AddFace(new_faces[pos_faces]);
                    meshPointer->AddFace(new_faces[pos_faces +1]);


                    cells.push_back(meshPointer->CreateCell());
                    cells[j]->AddPoint(points[0]);
                    cells[j]->AddPoint(points[1]);
                    cells[j]->AddPoint(long_edge.Point(0));
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
