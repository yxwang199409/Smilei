
#include "Timer.h"
#include "SmileiMPI.h"
#include "Tools.h"
#include <iomanip>

#include <mpi.h>
#include <string>

using namespace std;

Timer::Timer()
{
    smpi_ = NULL;
    name_ = "";
}

Timer::~Timer()
{
}

void Timer::init(SmileiMPI *smpi, string name)
{
    smpi_ = smpi;
    smpi_->barrier();
    last_start_ = MPI_Wtime();
    name_ = name;
}


void Timer::update()
{
    smpi_->barrier();
    time_acc_ +=  MPI_Wtime()-last_start_;
    last_start_ = MPI_Wtime();
}

void Timer::restart()
{
    smpi_->barrier();
    last_start_ = MPI_Wtime();
}

void Timer::print()
{
    if ((time_acc_>0.) && (name_!=""))
        MESSAGE(0, "\t" << setw(12) << name_ << "\t" << time_acc_ );
}
