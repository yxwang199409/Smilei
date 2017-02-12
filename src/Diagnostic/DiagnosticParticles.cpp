
#include "DiagnosticParticles.h"

#include <iomanip>

using namespace std;


// Constructor
DiagnosticParticles::DiagnosticParticles( Params &params, SmileiMPI* smpi, Patch* patch, int diagId )
{
    fileId_ = 0;
    
    int n_diag_particles = diagId;
    string type;
    double min, max;
    int nbins;
    bool logscale, edge_inclusive;
    
    ostringstream name("");
    name << "Diagnotic Particles #" << n_diag_particles;
    string errorPrefix = name.str();
    
    // get parameter "output" that determines the quantity to sum in the output array
    output = "";
    if (!PyTools::extract("output",output,"DiagParticles",n_diag_particles))
        ERROR(errorPrefix << ": parameter `output` required");
    
    // get parameter "every" which describes a timestep selection
    timeSelection = new TimeSelection(
        PyTools::extract_py("every", "DiagParticles", n_diag_particles),
        name.str()
    );
    
    // get parameter "flush_every" which describes a timestep selection for flushing the file
    flush_timeSelection = new TimeSelection(
        PyTools::extract_py("flush_every", "DiagParticles", n_diag_particles),
        name.str()
    );
    
    // get parameter "time_average" that determines the number of timestep to average the outputs
    time_average = 1;
    PyTools::extract("time_average",time_average,"DiagParticles",n_diag_particles);
    if ( time_average < 1 ) time_average=1;
    if ( time_average > timeSelection->smallestInterval() )
        ERROR(errorPrefix << ": `time_average` is incompatible with `every`");
    
    // get parameter "species" that determines the species to use (can be a list of species)
    vector<string> species_names;
    if (!PyTools::extract("species",species_names,"DiagParticles",n_diag_particles))
        ERROR(errorPrefix << ": parameter `species` required");
    // verify that the species exist, remove duplicates and sort by number
    species = params.FindSpecies(patch->vecSpecies, species_names);
    
    
    // get parameter "axes" that adds axes to the diagnostic
    // Each axis should contain several items:
    //      requested quantity, min value, max value ,number of bins, log (optional), edge_inclusive (optional)
    vector<PyObject*> pyAxes=PyTools::extract_pyVec("axes","DiagParticles",n_diag_particles);
    
    if (pyAxes.size() == 0)
        ERROR(errorPrefix << ": axes must contain something");
    
    // Loop axes and extract their format
    vector<HistogramAxis*> axes;
    output_size = 1;
    for (unsigned int iaxis=0; iaxis<pyAxes.size(); iaxis++ ) {
        PyObject *pyAxis=pyAxes[iaxis];
        
        // Axis must be a list
        if (!PyTuple_Check(pyAxis) && !PyList_Check(pyAxis))
            ERROR(errorPrefix << ": axis #" << iaxis << " must be a list");
        PyObject* seq = PySequence_Fast(pyAxis, "expected a sequence");
        
        // Axis must have 4 elements or more
        unsigned int lenAxisArgs=PySequence_Size(seq);
        if (lenAxisArgs<4)
            ERROR(errorPrefix << ": axis #" << iaxis << " contain at least 4 arguments");
        
        // Try to extract first element: type
        if (!PyTools::convert(PySequence_Fast_GET_ITEM(seq, 0), type))
            ERROR(errorPrefix << ", axis #" << iaxis << ": First item must be a string (axis type)");
        
        // Try to extract second element: axis min
        if (!PyTools::convert(PySequence_Fast_GET_ITEM(seq, 1), min)) {
            ERROR(errorPrefix << ", axis #" << iaxis << ": Second item must be a double (axis min)");
        }
        
        // Try to extract third element: axis max
        if (!PyTools::convert(PySequence_Fast_GET_ITEM(seq, 2), max)) {
            ERROR(errorPrefix << ", axis #" << iaxis << ": Third item must be a double (axis max)");
        }
        
        // Try to extract fourth element: axis nbins
        if (!PyTools::convert(PySequence_Fast_GET_ITEM(seq, 3), nbins)) {
            ERROR(errorPrefix << ", axis #" << iaxis << ": Fourth item must be an int (number of bins)");
        }
        
        // Check for  other keywords such as "logscale" and "edge_inclusive"
        logscale = false;
        edge_inclusive = false;
        for(unsigned int i=4; i<lenAxisArgs; i++) {
            string my_str("");
            PyTools::convert(PySequence_Fast_GET_ITEM(seq, i),my_str);
            if(my_str=="logscale" ||  my_str=="log_scale" || my_str=="log")
                logscale = true;
            else if(my_str=="edges" ||  my_str=="edge" ||  my_str=="edge_inclusive" ||  my_str=="edges_inclusive")
                edge_inclusive = true;
            else
                ERROR(errorPrefix << ": keyword `" << my_str << "` not understood");
        }
        
        HistogramAxis * axis;
        vector<double> coefficients(0);
        if        (type == "x" ) {
            axis = new HistogramAxis_x( type, min, max, nbins, logscale, edge_inclusive, coefficients );
        } else if (type == "y" ) {
            if (params.nDim_particle <2)
                ERROR(errorPrefix << ": axis y cannot exist in <2D");
            axis = new HistogramAxis_y( type, min, max, nbins, logscale, edge_inclusive, coefficients );
        } else if (type == "z" ) {
            if (params.nDim_particle <3)
                ERROR(errorPrefix << ": axis z cannot exist in <3D");
            axis = new HistogramAxis_z( type, min, max, nbins, logscale, edge_inclusive, coefficients );
        } else if (type == "px" ) {
            axis = new HistogramAxis_px( type, min, max, nbins, logscale, edge_inclusive, coefficients );
        } else if (type == "py" ) {
            axis = new HistogramAxis_py( type, min, max, nbins, logscale, edge_inclusive, coefficients );
        } else if (type == "pz" ) {
            axis = new HistogramAxis_pz( type, min, max, nbins, logscale, edge_inclusive, coefficients );
        } else if (type == "p" ) {
            axis = new HistogramAxis_p( type, min, max, nbins, logscale, edge_inclusive, coefficients );
        } else if (type == "gamma" ) {
            axis = new HistogramAxis_gamma( type, min, max, nbins, logscale, edge_inclusive, coefficients );
        } else if (type == "ekin" ) {
            axis = new HistogramAxis_ekin( type, min, max, nbins, logscale, edge_inclusive, coefficients );
        } else if (type == "vx" ) {
            axis = new HistogramAxis_vx( type, min, max, nbins, logscale, edge_inclusive, coefficients );
        } else if (type == "vy" ) {
            axis = new HistogramAxis_vy( type, min, max, nbins, logscale, edge_inclusive, coefficients );
        } else if (type == "vz" ) {
            axis = new HistogramAxis_vz( type, min, max, nbins, logscale, edge_inclusive, coefficients );
        } else if (type == "v" ) {
            axis = new HistogramAxis_v( type, min, max, nbins, logscale, edge_inclusive, coefficients );
        } else if (type == "vperp2" ) {
            axis = new HistogramAxis_vperp2( type, min, max, nbins, logscale, edge_inclusive, coefficients );
        } else if (type == "charge" ) {
            axis = new HistogramAxis_charge( type, min, max, nbins, logscale, edge_inclusive, coefficients );
        } else if (type == "chi" ) {
            // The requested species must be radiating
            for (unsigned int ispec=0 ; ispec < species.size() ; ispec++)
                if( ! patch->vecSpecies[species[ispec]]->particles->isRadReaction )
                    ERROR(errorPrefix << ": axis #" << iaxis << " 'chi' requires all species to be 'radiating'");
            axis = new HistogramAxis_chi( type, min, max, nbins, logscale, edge_inclusive, coefficients );
        } else if (type == "composite") {
            ERROR(errorPrefix << ": axis type cannot be 'composite'");
        
        } else {
            // If not "usual" type, try to find composite type
            for( unsigned int i=1; i<=type.length(); i++ )
                if( type.substr(i,1) == " " )
                    ERROR(errorPrefix << ": axis #" << iaxis << " type cannot contain whitespace");
            if( type.length()<2 )
                ERROR(errorPrefix << ": axis #" << iaxis << " type not understood");
            
            // Analyse character by character
            coefficients.resize( params.nDim_particle , 0. );
            unsigned int previ=0;
            double sign=1.;
            type += "+";
            for( unsigned int i=1; i<=type.length(); i++ ) {
                // Split string at "+" or "-" location
                if( type.substr(i,1) == "+" || type.substr(i,1) == "-" ) {
                    // Get one segment of the split string
                    string segment = type.substr(previ,i-previ);
                    // Get the last character, which should be one of x, y, or z
                    unsigned int j = segment.length();
                    string direction = j>0 ? segment.substr(j-1,1) : "";
                    unsigned int direction_index;
                    if     ( direction == "x" ) direction_index = 0;
                    else if( direction == "y" ) direction_index = 1;
                    else if( direction == "z" ) direction_index = 2;
                    else { ERROR(errorPrefix << ": axis #" << iaxis << " type not understood"); }
                    if( direction_index >= params.nDim_particle )
                        ERROR(errorPrefix << ": axis #" << iaxis << " type " << direction << " cannot exist in " << params.nDim_particle << "D");
                    if( coefficients[direction_index] != 0. )
                        ERROR(errorPrefix << ": axis #" << iaxis << " type " << direction << " appears twice");
                    // Get the remaining characters, which should be a number
                    coefficients[direction_index] = j>1 ? ::atof(segment.substr(0,j-1).c_str()) : 1.;
                    coefficients[direction_index] *= sign;
                    // Save sign and position for next segment
                    sign = type.substr(i,1) == "+" ? 1. : -1;
                    previ = i+1;
                }
            }
            
            type = "composite:"+type.substr(0,type.length()-1);
            axis = new HistogramAxis_composite( type, min, max, nbins, logscale, edge_inclusive, coefficients );
        }
        
        Py_DECREF(seq);
        
        output_size *= nbins; // total array size
        axes.push_back( axis );
    }
    
    // Create the Histogram object
    if        (output == "density"        ) {
        histogram = new Histogram_density(axes);
    } else if (output == "charge_density" ) {
        histogram = new Histogram_charge_density(axes);
    } else if (output == "jx_density"     ) {
        histogram = new Histogram_jx_density(axes);
    } else if (output == "jy_density"     ) {
        histogram = new Histogram_jy_density(axes);
    } else if (output == "jz_density"     ) {
        histogram = new Histogram_jz_density(axes);
    } else if (output == "ekin_density"   ) {
        histogram = new Histogram_ekin_density(axes);
    } else if (output == "p_density"      ) {
        histogram = new Histogram_p_density(axes);
    } else if (output == "px_density"     ) {
        histogram = new Histogram_px_density(axes);
    } else if (output == "py_density"     ) {
        histogram = new Histogram_py_density(axes);
    } else if (output == "pz_density"     ) {
        histogram = new Histogram_pz_density(axes);
    } else if (output == "pressure_xx"    ) {
        histogram = new Histogram_pressure_xx(axes);
    } else if (output == "pressure_yy"    ) {
        histogram = new Histogram_pressure_yy(axes);
    } else if (output == "pressure_zz"    ) {
        histogram = new Histogram_pressure_zz(axes);
    } else if (output == "pressure_xy"    ) {
        histogram = new Histogram_pressure_xy(axes);
    } else if (output == "pressure_xz"    ) {
        histogram = new Histogram_pressure_xz(axes);
    } else if (output == "pressure_yz"    ) {
        histogram = new Histogram_pressure_yz(axes);
    } else if (output == "ekin_vx_density") {
        histogram = new Histogram_ekin_vx_density(axes);
    } else {
        ERROR(errorPrefix << ": parameter `output = "<< output <<"` not understood");
    }
    
    // Output info on diagnostics
    if ( smpi->isMaster() ) {
        ostringstream mystream("");
        mystream.str("");
        mystream << species_names[0];
        for(unsigned int i=1; i<species_names.size(); i++)
            mystream << "," << species_names[i];
        MESSAGE(1,"Created particle diagnostic #" << n_diag_particles << ": species " << mystream.str());
        for(unsigned int i=0; i<axes.size(); i++) {
            mystream.str("");
            mystream << "Axis ";
            if( axes[i]->type.substr(0,9) == "composite" ) {
                bool first = true;
                for( unsigned int idim=0; idim<axes[i]->coefficients.size(); idim++ ) {
                    if( axes[i]->coefficients[idim]==0. ) continue;
                    bool negative = axes[i]->coefficients[idim]<0.;
                    double coeff = (negative?-1.:1.)*axes[i]->coefficients[idim];
                    mystream << (negative?"-":(first?"":"+"));
                    if( coeff!=1. ) mystream << coeff;
                    mystream << (idim==0?"x":(idim==1?"y":"z"));
                    first = false;
                }
            } else {
                mystream << axes[i]->type;
            }
            mystream << " from " << axes[i]->min << " to " << axes[i]->max << " in " << axes[i]->nbins << " steps";
            if( axes[i]->logscale       ) mystream << " [LOGSCALE] ";
            if( axes[i]->edge_inclusive ) mystream << " [EDGE INCLUSIVE]";
            MESSAGE(2,mystream.str());
        }
        
        // init HDF files (by master, only if it doesn't yet exist)
        mystream.str(""); // clear
        mystream << "ParticleDiagnostic" << n_diag_particles << ".h5";
        filename = mystream.str();
    }

} // END DiagnosticParticles::DiagnosticParticles


DiagnosticParticles::~DiagnosticParticles()
{
    delete timeSelection;
    delete flush_timeSelection;
} // END DiagnosticParticles::~DiagnosticParticles


// Called only by patch master of process master
void DiagnosticParticles::openFile( Params& params, SmileiMPI* smpi, bool newfile )
{
    if (!smpi->isMaster()) return;
    
    if( fileId_>0 ) return;
    
    if ( newfile ) {
        fileId_ = H5Fcreate( filename.c_str(), H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);
        // write all parameters as HDF5 attributes
        H5::attr(fileId_, "Version", string(__VERSION));
        H5::attr(fileId_, "output" , output);
        H5::attr(fileId_, "time_average"  , time_average);
        // write all species
        ostringstream mystream("");
        mystream.str(""); // clear
        for (unsigned int i=0 ; i < species.size() ; i++)
            mystream << species[i] << " ";
        H5::attr(fileId_, "species", mystream.str());
        // write each axis
        for (unsigned int iaxis=0 ; iaxis < histogram->axes.size() ; iaxis++) {
            mystream.str(""); // clear
            mystream << "axis" << iaxis;
            string str1 = mystream.str();
            mystream.str(""); // clear
            mystream << histogram->axes[iaxis]->type << " " << histogram->axes[iaxis]->min << " " << histogram->axes[iaxis]->max << " "
                     << histogram->axes[iaxis]->nbins << " " << histogram->axes[iaxis]->logscale << " " << histogram->axes[iaxis]->edge_inclusive << " [";
            for( unsigned int idim=0; idim<histogram->axes[iaxis]->coefficients.size(); idim++) {
                mystream << histogram->axes[iaxis]->coefficients[idim];
                if(idim<histogram->axes[iaxis]->coefficients.size()-1) mystream << ",";
            }
            mystream << "]";
            string str2 = mystream.str();
            H5::attr(fileId_, str1, str2);
        }
        H5Fflush( fileId_, H5F_SCOPE_GLOBAL );
    }
    else {
        fileId_ = H5Fopen(filename.c_str(), H5F_ACC_RDWR, H5P_DEFAULT);
    }
}


void DiagnosticParticles::closeFile()
{
    if (fileId_!=0) {
        H5Fclose(fileId_);
        fileId_ = 0;
    }

} // END closeFile


bool DiagnosticParticles::prepare( int timestep )
{
    // Get the previous timestep of the time selection
    int previousTime = timeSelection->previousTime(timestep);
    
    // Leave if the timestep is not the good one
    if (timestep - previousTime >= time_average) return false;
    
    // Allocate memory for the output array (already done if time-averaging)
    data_sum.resize(output_size);
    
    // if first time, erase output array
    if (timestep == previousTime)
        fill(data_sum.begin(), data_sum.end(), 0.);
    
    return true;
    
} // END prepare


// run one particle diagnostic
void DiagnosticParticles::run( Patch* patch, int timestep )
{
    
    vector<int> int_buffer;
    vector<double> double_buffer;
    unsigned int npart;
    
    // loop species
    for (unsigned int ispec=0 ; ispec < species.size() ; ispec++) {
        
        Species * s = patch->vecSpecies[species[ispec]];
        npart = s->particles->size();
        int_buffer   .resize(npart);
        double_buffer.resize(npart);
        
        histogram->digitize( s, double_buffer, int_buffer, data_sum );
        
    }
    
} // END run


// Now the data_sum has been filled
// if needed now, store result to hdf file
void DiagnosticParticles::write(int timestep, SmileiMPI* smpi)
{
    if ( !smpi->isMaster() ) return;
    
    if (timestep - timeSelection->previousTime() != time_average-1) return;
    
    double coeff;
    // if time_average, then we need to divide by the number of timesteps
    if (time_average > 1) {
        coeff = 1./((double)time_average);
        for (int i=0; i<output_size; i++)
            data_sum[i] *= coeff;
    }
    
    // make name of the array
    ostringstream mystream("");
    mystream.str("");
    mystream << "timestep" << setw(8) << setfill('0') << timestep;
    
    // write the array if it does not exist already
    if (! H5Lexists( fileId_, mystream.str().c_str(), H5P_DEFAULT ) ) {
        // Prepare array dimensions
        unsigned int naxes = histogram->axes.size();
        hsize_t dims[naxes];
        for( unsigned int iaxis=0; iaxis<naxes; iaxis++) dims[iaxis] = histogram->axes[iaxis]->nbins;
        // Create file space
        hid_t sid = H5Screate_simple(naxes, &dims[0], NULL);
        hid_t pid = H5Pcreate(H5P_DATASET_CREATE);
        // create dataset
        hid_t did = H5Dcreate(fileId_, mystream.str().c_str(), H5T_NATIVE_DOUBLE, sid, H5P_DEFAULT, pid, H5P_DEFAULT);
        // write vector in dataset
        H5Dwrite(did, H5T_NATIVE_DOUBLE, sid, sid, H5P_DEFAULT, &data_sum[0]);
        // close all
        H5Dclose(did);
        H5Pclose(pid);
        H5Sclose(sid);
    }
    
    if( flush_timeSelection->theTimeIsNow(timestep) ) H5Fflush( fileId_, H5F_SCOPE_GLOBAL );
    
    // Clear the array
    clear();
    data_sum.resize(0);
} // END write


//! Clear the array
void DiagnosticParticles::clear() {
    data_sum.resize(0);
    vector<double>().swap( data_sum );
}
