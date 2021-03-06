#include "coordinategroup.h"
#include "universe.h"


coordinategroup::coordinategroup(std::vector<double>& coords)
{
    int problemdimension = universe::mymesh->getmeshdimension();
    mynumcoords = coords.size()/3;
    
    noisethreshold = universe::mymesh->getnodes()->getnoisethreshold();
    double meshsize = 0.0;
    for (int i = 0; i < 3; i++)
        meshsize += universe::mymesh->getnodes()->getgeometrydimension(i);
    
    // Define the number of slices in the x, y and z direction:
    int numblocks = std::ceil((double)mynumcoords/N);
    int ns = std::ceil( std::pow(numblocks, 1.0/problemdimension) );
    numslices = {ns,ns,ns};
    
    // Get the coordinate x, y and z bounds as well as the distance between slices:
    bounds = myalgorithm::getcoordbounds(coords);
    delta = {(bounds[1]-bounds[0])/numslices[0], (bounds[3]-bounds[2])/numslices[1], (bounds[5]-bounds[4])/numslices[2]};
    // This solves the non-existing dimension issues:
    for (int i = 0; i < 3; i++)
    {
        if (delta[i] < 1e-6*meshsize)
        {
            delta[i] = 2.0*meshsize;
            numslices[i] = 1;
        }
    }
    
    std::vector<double> xcoords(mynumcoords), ycoords(mynumcoords), zcoords(mynumcoords);
    for (int i = 0; i < mynumcoords; i++)
    {
        xcoords[i] = coords[3*i+0];
        ycoords[i] = coords[3*i+1];
        zcoords[i] = coords[3*i+2];
    }
    
    // Preallocate 'mygroups' and 'mygroupcoords':
    mygroups = std::vector<std::vector<std::vector<std::vector<int>>>>(numslices[0], std::vector<std::vector<std::vector<int>>>(numslices[1], std::vector<std::vector<int>>(numslices[2], std::vector<int>(0))));
    mygroupcoords = std::vector<std::vector<std::vector<std::vector<double>>>>(numslices[0], std::vector<std::vector<std::vector<double>>>(numslices[1], std::vector<std::vector<double>>(numslices[2], std::vector<double>(0))));
        

    std::vector<std::vector<int>> xslices;
    myalgorithm::slicecoordinates(noisethreshold[0], xcoords, bounds[0], delta[0], numslices[0], xslices);
    
    // Loop on all x slices:
    for (int i = 0; i < numslices[0]; i++)
    {
        // Get the y coordinates vector for the current slice:
        std::vector<double> curycoords(xslices[i].size());
        for (int j = 0; j < xslices[i].size(); j++)
            curycoords[j] = ycoords[xslices[i][j]];
            
        // Slice the current slice in y slices:
        std::vector<std::vector<int>> yslices;
        myalgorithm::slicecoordinates(noisethreshold[1], curycoords, bounds[2], delta[1], numslices[1], yslices);
        
        // Loop on all y slices:
        for (int j = 0; j < numslices[1]; j++)
        {
            // Get the z coordinates vector for the current slice:
            std::vector<double> curzcoords(yslices[j].size());
            for (int k = 0; k < yslices[j].size(); k++)
                curzcoords[k] = zcoords[xslices[i][yslices[j][k]]];
                
            // Slice the current slice in z slices:
            std::vector<std::vector<int>> zslices;
            myalgorithm::slicecoordinates(noisethreshold[2], curzcoords, bounds[4], delta[2], numslices[2], zslices);
            
            for (int k = 0; k < numslices[2]; k++)
            {
                // Populate the groups:
                mygroups[i][j][k] = std::vector<int>(zslices[k].size());
                for (int l = 0; l < zslices[k].size(); l++)
                    mygroups[i][j][k][l] = xslices[i][yslices[j][zslices[k][l]]];
            
                mygroupcoords[i][j][k] = std::vector<double>(3*zslices[k].size());
                for (int l = 0; l < zslices[k].size(); l++)
                {
                    int cur = mygroups[i][j][k][l];
                    mygroupcoords[i][j][k][3*l+0] = coords[3*cur+0];
                    mygroupcoords[i][j][k][3*l+1] = coords[3*cur+1];
                    mygroupcoords[i][j][k][3*l+2] = coords[3*cur+2];
                }
            }
        }
    }
}


void coordinategroup::select(double x, double y, double z, double r)
{
    xselect = x;
    yselect = y;
    zselect = z;
    myradius = r;
    
    // Take an extra noise margin to be sure not to miss any candidate slice:
    int x1 = std::max( (int)std::floor( ( x-r-noisethreshold[0] - bounds[0] )/delta[0] ), 0);
    int x2 = std::min( (int)std::ceil( ( x+r+noisethreshold[0] - bounds[0] )/delta[0] ) - 1, numslices[0]-1);
    int y1 = std::max( (int)std::floor( ( y-r-noisethreshold[1] - bounds[2] )/delta[1] ), 0);
    int y2 = std::min( (int)std::ceil( ( y+r+noisethreshold[1] - bounds[2] )/delta[1] ) - 1, numslices[1]-1);
    int z1 = std::max( (int)std::floor( ( z-r-noisethreshold[2] - bounds[4] )/delta[2] ), 0);
    int z2 = std::min( (int)std::ceil( ( z+r+noisethreshold[2] - bounds[4] )/delta[2] ) - 1, numslices[2]-1);
    
    int numx = 0, numy = 0, numz = 0;;
    for (int i = x1; i <= x2; i++)
        numx++;
    for (int i = y1; i <= y2; i++)
        numy++;
    for (int i = z1; i <= z2; i++)
        numz++;
    
    selectedgroups.resize(3*numx*numy*numz);
    
    int index = 0;
    for (int i = x1; i <= x2; i++)
    {
        for (int j = y1; j <= y2; j++)
        {
            for (int k = z1; k <= z2; k++)
            {
                selectedgroups[3*index+0] = i;
                selectedgroups[3*index+1] = j;
                selectedgroups[3*index+2] = k;
                index++;
            }
        }
    }
}

int coordinategroup::countcoordinates(void)
{
    return mynumcoords;
}

int coordinategroup::countgroups(void)
{
    return selectedgroups.size()/3;
}

std::vector<int>* coordinategroup::getgroupindexes(int g)
{
    return &(mygroups[selectedgroups[3*g+0]][selectedgroups[3*g+1]][selectedgroups[3*g+2]]);
}

std::vector<double>* coordinategroup::getgroupcoordinates(int g)
{
    return &(mygroupcoords[selectedgroups[3*g+0]][selectedgroups[3*g+1]][selectedgroups[3*g+2]]);
}


