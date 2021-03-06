

#include <RcppArmadillo.h>
#include <math.h> 
#include <omp.h>
// [[Rcpp::plugins(openmp)]]


// [[Rcpp::depends(RcppArmadillo)]]
using namespace Rcpp;
using namespace arma;
// [[Rcpp::export]]


double ST1a(double z,double gam){
double sparse=0;
if(z>0 && gam<fabs(z)) return(z-gam);

if(z<0 && gam<fabs(z)) return(z+gam);
if(gam>=fabs(z)) return(sparse);
else return(0);
}

// [[Rcpp::export]]
colvec ST3a(colvec z ,double gam)
{
int n=z.size();
colvec z1(n);
for( int i=0; i<n;++i)
{
double z11=z(i);
z1(i)=ST1a(z11,gam);
}
return(z1);
}

uvec ind(int n2,int m){
IntegerVector subs(n2);
for(int i =0 ; i<n2;++i)
{
subs(i)=i;
 }
subs.erase(m);
return(as<uvec>(subs));
}


// [[Rcpp::export]]
mat lassocore(const mat Y,const mat Z, mat B,mat BOLD, const double gam,const double lassothresh,const colvec ZN, uvec m, const int k2,const int n2){
double thresh=10*lassothresh;

while(thresh>lassothresh)
{
 int j;
  for(j = 0; j < k2; ++j)
	{
	
	  
// Required critical region due to the use of an Rcpp object.         
      	#pragma omp critical
       	  {

     	    m=ind(k2,j);	       
	  }
	  B.col(j)=ST3a((Y-B.cols(m)*Z.rows(m))*trans(Z.row(j)),gam)/ZN(j);
	}
mat thresh1=abs((B-BOLD)/(ones(n2,k2)+abs(BOLD)));
 thresh=norm(thresh1,"inf");
 BOLD=B;
  
}

return(B);
}
// [[Rcpp::export]]
cube gamloopP(NumericVector beta_,const mat Y,const mat Z, const colvec gammgrid, const double lassothresh,const colvec YMean2,const colvec ZMean2,const colvec znorm,mat BFoo){

  //Data is read in from R as Armadillo objects directly.  Initially, I did not know that this was possible


//setting number of threads
omp_set_num_threads(4);

 // set stacksize, not really necessary unless you are running into memory issues
setenv("OMP_STACKSIZE","64M",1);


const int n2=BFoo.n_rows;
const int k2=BFoo.n_cols;

//Coefficient matrices
 mat B1=BFoo;
 mat B1F2=BFoo;
 mat b2=B1;
const int ngridpts=gammgrid.size();

 //These are typically defined inside of the "lassocore" function, but doing so causes problems with openMP, so they are defined out here and read in to the function
// const int ngridpts=gamm.size();
cube bcube(beta_.begin(),n2,k2,ngridpts,false);
cube bcube2(n2,k2+1,ngridpts);
bcube2.fill(0.0);
// const colvec ZN = as<colvec>(znorm2);
colvec nu=zeros<colvec>(n2);
uvec m=ind(k2,1);
double gam =0;
 int i;
#pragma omp parallel for shared(bcube,bcube2,b2) private(i,gam,B1F2,B1,m,nu) default(none) schedule(auto)
  for (i=0; i<ngridpts;++i) {
      gam=gammgrid(i);
        //Previous coefficient matrix is read in as a "warm start"

	mat B1F2=bcube.slice(i);
	//The actual algorithm is being applied here 
	 	B1 = lassocore(Y,Z,B1F2,b2,gam, lassothresh,znorm,m,k2,n2); 
	

	//intercept is calculated
	 nu = YMean2 - B1 *ZMean2;
         bcube2.slice(i) = mat(join_horiz(nu, B1)); 
	}
    return(bcube2);
}




