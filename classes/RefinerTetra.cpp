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

        CutterMesh3D cutter;
        cutter.SetMesh(*meshPointer);
        for(int i = 0; i < long_edge.NumberOfCells(); i++)
        {
            if (long_edge.Cell(i) != &cell)
            {
                Vector3d a[2];
                int j = 0;
                for(int i = 0; i < cell.NumberOfPoints() ; i++)
                {
                    const GenericPoint* current_point = cell.Point(i);
                    if(current_point != long_edge.Point(0) && current_point != long_edge.Point(1))
                    {
                        a[j] = current_point -> Coordinates() - new_point;
                        j++;
                    }
                }
                Vector3d normal = a[0].cross(a[1]);
                normal.normalize();
                double translation = normal.dot(new_point);
                cutter.intersector2D1D = new Intersector2D1D;
                Intersector2D1D& intersector = *cutter.intersector2D1D;
			    intersector.SetPlane(normal, translation);
                cutter.CutCell(cell, normal, translation, 1.0E-7);
            }
        }

    }

    const Output::ExitCodes RefinerTetra::RefineMesh()
    {
        cout << idCellToRefine << idCellToRefine.size() << endl;

        for(int i = 0; i < idCellToRefine.size(); i++)
        {
            CutTetra(*meshPointer -> Cell(idCellToRefine[i]));
        }
        return Output::Success;
    }
}
