// 
//     MINOTAUR -- It's only 1/2 bull
// 
//     (C)opyright 2009 - 2017 The MINOTAUR Team.
// 

/** 
 * \file Linearizations.cpp
 * \Briefly define a class for adding linearizations in linearization
 * based methods. Added for problems with nonlinear constraints
 * \Author Meenarli Sharma, Indian Institute of Technology Bombay
 */


#include <cmath>
#include <string>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <iostream>
#include <algorithm>

#include "MinotaurConfig.h"

#include "CNode.h"
#include "Constraint.h"
#include "Engine.h"
#include "Environment.h"
#include "Function.h"
#include "Logger.h"
#include "Node.h"
#include "NonlinearFunction.h"
#include "QuadraticFunction.h"
#include "Objective.h"
#include "Operations.h"
#include "Option.h"
#include "ProblemSize.h"
#include "Linearizations.h"
#include "Relaxation.h"
#include "Solution.h"
#include "SolutionPool.h"
#include "VarBoundMod.h"
#include "Variable.h"

using namespace Minotaur;

typedef std::vector<ConstraintPtr>::const_iterator CCIter;
const std::string Linearizations::me_ = "Linearizations: ";

Linearizations::Linearizations(EnvPtr env, RelaxationPtr rel,
                               ProblemPtr minlp, std::vector<ConstraintPtr> nlCons)
: env_(env),
  rel_(rel),
  minlp_(minlp),
  //lpe_(EnginePtr()),
  nlpe_(EnginePtr()),
  solC_(NULL),
  nlpx_(NULL),
  varPtrs_(0)
{
  nlCons_ = nlCons;
  logger_ = env->getLogger();
  //MS: set the option for root_LinSchemes
  rs1_ = env_->getOptions()->findDouble("root_linScheme1")->getValue();
  rs2Per_ = env_->getOptions()->findDouble("root_linScheme2_per")->getValue();
  rs2NbhSize_ = env_->getOptions()->findDouble("root_linScheme2_nbhSize")->getValue();
  rs3_ = env_->getOptions()->findInt("root_linScheme3")->getValue();
  rgs1_ = env_->getOptions()->findBool("root_genLinScheme1")->getValue();
  rgs2_ = env_->getOptions()->findBool("root_genLinScheme2")->getValue();
  intTol_ = env_->getOptions()->findDouble("int_tol")->getValue();
  solAbsTol_ = env_->getOptions()->findDouble("feasAbs_tol")->getValue();
  solRelTol_ = env_->getOptions()->findDouble("feasRel_tol")->getValue();
  objATol_ = env_->getOptions()->findDouble("solAbs_tol")->getValue();
  objRTol_ = env_->getOptions()->findDouble("solRel_tol")->getValue();

  stats_ = new LinStats();
  stats_->rs1Cuts = 0;
  stats_->rs2Cuts = 0;
  stats_->rs3Cuts = 0;
  stats_->rgs1Cuts = 0;
  stats_->rgs2Cuts = 0;
 }


Linearizations::~Linearizations()
{ 
  if (stats_) {
    delete stats_;
  }
  if (solC_) {
    delete [] solC_;
    solC_ = 0;
  }
  env_ = 0;
  rel_ = 0;
  minlp_ = 0;
  nlCons_.clear();
}


bool Linearizations::addCutAtRoot_(double *x, ConstraintPtr con, UInt &newConId)
{
  int error = 0; 
  FunctionPtr f;
  double c, act, cUb;
  ConstraintPtr newcon;
  std::stringstream sstm;
  LinearFunctionPtr lf = LinearFunctionPtr();

  act = con->getActivity(x, &error);
  if (error == 0) {
    f = con->getFunction();
    linearAt_(f, act, x, &c, &lf, &error);
    if (error == 0) {
      cUb = con->getUb();
      ++(stats_->rs1Cuts);
      sstm << "_OAcut_" << stats_->rs1Cuts << "_AtRoot";
      f = (FunctionPtr) new Function(lf);
      newcon = rel_->newConstraint(f, -INFINITY, cUb-c, sstm.str());
      newConId = newcon->getIndex();
      sstm.str("");
      return true;
    }
  }	else {
    logger_->msgStream(LogError) << me_ << "Constraint" <<  con->getName() <<
      " is not defined at this point." << std::endl;
  }
  return false;
}

//MS: delete unused functions
bool Linearizations::cutAtLineSearchPt_(const double *xout, double* xnew,
                                        ConstraintPtr con)
{
  double nlpact;
  bool lsPtFound;
  lsPtFound = lineSearchPt_(xnew, xout, con, nlpact);
  if (lsPtFound) {
    int error = 0;
    std::stringstream sstm;
    //ConstraintPtr newcon;
    LinearFunctionPtr lf = 0;
    FunctionPtr f = con->getFunction();
    double c, cUb = con->getUb();
    if (error == 0) {
      linearAt_(f, nlpact, xnew, &c, &lf, &error);
      ++(stats_->rs3Cuts);
      sstm << "_OAcut_" << stats_->rs3Cuts;
      f = (FunctionPtr) new Function(lf);
      rel_->newConstraint(f, -INFINITY, cUb-c, sstm.str());
      //newcon = rel_->newConstraint(f, -INFINITY, cUb-c, sstm.str());
    }	else {
      return false;
    }
   } else {
     return false;   
   }
  return true;
}

//MS: for rootLinScheme3_ for all constraints
//use here cutAtLineSearchPt()
//void Linearizations::addEshAtRoot_(const double *lpx, double* x, ConstraintPtr con)
//{
  //int error = 0;
  //bool lsPtFound;
  //std::stringstream sstm;
  ////ConstraintPtr newcon;
  //LinearFunctionPtr lf = 0;
  //FunctionPtr f = con->getFunction();
  //double c, nlpact, cUb = con->getUb();
  
  //lsPtFound = lineSearchPt_(x, solC_, lpx, con, nlpact);

  //if (lsPtFound) {
    //if (error == 0) {
      //linearAt_(f, nlpact, x, &c, &lf, &error);
    //}	else {
      //return;
    //}
  //} else {
    //nlpact =  con->getActivity(lpx, &error);
    //if (error == 0) {
      //linearAt_(f, nlpact, lpx, &c, &lf, &error);
    //}	else {
      //return;
    //}
  //}
  //if (error==0) {
    ////++(stats_->cuts);
    ////sstm << "_OAcut_" << stats_->cuts;
    //f = (FunctionPtr) new Function(lf);
    //rel_->newConstraint(f, -INFINITY, cUb-c, sstm.str());
    ////newcon = rel_->newConstraint(f, -INFINITY, cUb-c, sstm.str());
    ////newcon->write(std::cout);
  //}
  
  ////if (oNl_) {
    ////ObjectivePtr o = minlp_->getObjective();
    
    ////nlpact = o->eval(lpx, &error);
    ////if (error == 0) {
      ////vio = std::max(nlpact-relobj_, 0.0);
      ////if ((vio > solAbsTol_)
        ////&& (relobj_ == 0 || vio > fabs(relobj_)*solRelTol_)) {
        ////nlpact = o->eval(x, &error);
        ////if (error == 0) {
          ////f = o->getFunction();
          ////lf = LinearFunctionPtr(); 
          ////linearAt_(f, nlpact, x, &c, &lf, &error);
          ////if (error == 0) {
            ////vio = std::max(c+lf->eval(lpx)-relobj_, 0.0);
            ////if ((vio > solAbsTol_) && ((relobj_-c) == 0
                                     ////|| vio > fabs(relobj_-c)*solRelTol_)) {
              ////++(stats_->cuts);
              ////sstm << "_OAObjcut_" << stats_->cuts;
              ////lf->addTerm(objVar_, -1.0);
              ////f = (FunctionPtr) new Function(lf);
              ////rel_->newConstraint(f, -INFINITY, -1.0*c, sstm.str());
              //////newcon = rel_->newConstraint(f, -INFINITY, -1.0*c, sstm.str());
              //////newcon->write(std::cout);
            ////}
          ////}
        ////}
      ////}  else {

      ////}
    ////}	else {

    ////}
  ////}
  //return;
//}


bool Linearizations::linPart_(double *b1, UInt lVarIdx, ConstraintPtr con,
                           double lVarCoeff, double act)
{
  int error = 0;
  double nlTerm = 0; 
  QuadraticFunctionPtr qf = con->getQuadraticFunction();
  NonlinearFunctionPtr nlf = con->getNonlinearFunction();

  if (nlf) {
    nlTerm = nlf->eval(b1, &error); 
  } 

  if (error == 0) {
    if (qf) {
      nlTerm = nlTerm + qf->eval(b1); 
    }
    b1[lVarIdx] = (con->getUb() - nlTerm - act)/lVarCoeff;    
    return true;
  }
  return false;
}


bool Linearizations::addNewCut_(double *b1, ConstraintPtr con,
                           UInt &newConId)
{
  bool found;  
  found = addCutAtRoot_(b1, con, newConId);
  if (found) {
    return true;
  } 
  return false;
}


void Linearizations::findCenter()
{
  // Center is found if the feasible region is compact and has
  // non-empty inetrior. Otherwise, not.
  double lb, ub;
  VariablePtr vPtr, v;
  ConstraintPtr con;
  FunctionType fType;
  const double *dtemp;
  EngineStatus nlpStatus;
  std::vector<ConstraintPtr > cp;
  ProblemPtr inst_C = minlp_->clone();
  //inst_C->write(std::cout);

  FunctionPtr fnewc;
  LinearFunctionPtr lfc = (LinearFunctionPtr) new LinearFunction();
  vPtr = inst_C->newVariable(-INFINITY, 0, Continuous, "eta", VarHand);
  vPtr->setFunType_(Nonlinear);
  inst_C->removeObjective();
  lfc->addTerm(vPtr, 1.0);
  fnewc = (FunctionPtr) new Function(lfc);
  inst_C->newObjective(fnewc, 0.0, Minimize);

  for (ConstraintConstIterator it=inst_C->consBegin(); it!=inst_C->consEnd();
     ++it) {
    con = *it;
    lb = con->getLb();
    ub = con->getUb();
    fType = con->getFunctionType();
    if (fType == Constant) {
      continue;
    } else if (fType == Linear)  {
      if (lb != -INFINITY && ub != INFINITY) {
        if (lb == ub) {
          continue;       
        }
        cp.push_back(con);
        inst_C->markDelete(con);
        continue;
      } else if (lb != -INFINITY) {
        ub = INFINITY;
        lfc = con->getLinearFunction()->clone();
        lfc->addTerm(vPtr, 1.0);
      } else if (ub != INFINITY ) {
        lb = -INFINITY;
        lfc = con->getLinearFunction()->clone();
        lfc->addTerm(vPtr, -1.0);
      } else {
        inst_C->markDelete(con);
        continue;
      }
    } else {
      if (con->getLinearFunction()) {
        lfc = con->getLinearFunction()->clone();
        lfc->addTerm(vPtr, -1.0);
      } else {
        lfc = (LinearFunctionPtr) new LinearFunction();
        lfc->addTerm(vPtr, -1.0);
      }
    }
    inst_C->changeConstraint(con, lfc, lb, ub);
  }  

  for (UInt i = 0; i < cp.size(); ++i) {
    con = cp[i];
    lb = con->getLb(), ub = con->getUb();
    lfc = con->getLinearFunction()->clone();
    lfc->addTerm(vPtr, 1.0);
    fnewc = (FunctionPtr) new Function(lfc);
    inst_C->newConstraint(fnewc, lb, INFINITY);

    lfc = con->getLinearFunction()->clone();
    lfc->addTerm(vPtr, -1.0);
    fnewc = (FunctionPtr) new Function(lfc);
    inst_C->newConstraint(fnewc, -INFINITY, ub);
  }
  cp.clear();
  inst_C->delMarkedCons();

  for (VariableConstIterator vit=inst_C->varsBegin(); vit!=inst_C->varsEnd()-1;
       ++vit) {
    v = *vit;
    lb = v->getLb(), ub = v->getUb();
    if (lb == ub) {
      continue;
    }

    if (lb != -INFINITY) {
      lfc = (LinearFunctionPtr) new LinearFunction();
      lfc->addTerm(vPtr, 1.0);
      lfc->addTerm(v, 1.0);
      fnewc = (FunctionPtr) new Function(lfc);
      inst_C->newConstraint(fnewc, lb, INFINITY);
    }
    
    if (ub != INFINITY) {
      lfc = (LinearFunctionPtr) new LinearFunction();
      lfc->addTerm(vPtr, -1.0);
      lfc->addTerm(v, 1.0);
      fnewc = (FunctionPtr) new Function(lfc);
      inst_C->newConstraint(fnewc, -INFINITY, ub);
    }
  }

  inst_C->prepareForSolve();
  nlpe_->load(inst_C);
  nlpStatus = nlpe_->solve();
  //inst_C->write(std::cout);
 
  switch(nlpStatus) {
  case (ProvenOptimal):
  case (ProvenLocalOptimal):
    if (solC_) {
      delete solC_;
      solC_ = 0;
    }
    //std::cout << "Center " << nlpe_->getSolution()->getObjValue();
    if (nlpe_->getSolution()->getObjValue() < 0) {
      dtemp = nlpe_->getSolution()->getPrimal();
      solC_ = new double[minlp_->getNumVars()];
      std::copy(dtemp, dtemp+minlp_->getNumVars(), solC_);
    } 
    break;
  case (EngineIterationLimit):
  case (ProvenInfeasible):
  case (ProvenLocalInfeasible): 
  case (ProvenObjectiveCutOff):
    break;
  case (FailedFeas):
  case (EngineError):
  case (FailedInfeas):
  case (ProvenUnbounded):
  case (ProvenFailedCQFeas):
  case (EngineUnknownStatus):
  case (ProvenFailedCQInfeas):
  default:
    logger_->msgStream(LogError) << me_ << "NLP engine status = "
      << nlpe_->getStatusString() << std::endl;
    break;
  }
  delete nlpe_;
  nlpe_ = 0;
  delete inst_C;
  inst_C = 0;
  return;
}


bool Linearizations::findIntersectPt_(std::vector<UInt > newConsId, VariablePtr vl,
                                VariablePtr vnl, double * iP)
{
  ConstraintPtr con;
  LinearFunctionPtr lf;
  con = rel_->getConstraint(newConsId[0]);
  lf = con->getLinearFunction();
  //std::cout << "linear and nonlinear  pointer " << vl << " " << vnl << std::endl;
  double a = lf->getWeight(vl);
  double b = lf->getWeight(vnl);
  double e = con->getUb();
  
  con = rel_->getConstraint(newConsId[1]);
  lf = con->getLinearFunction();
  double c = lf->getWeight(vl);
  double d = lf->getWeight(vnl);
  double f = con->getUb();

  /* we solve the linear system
   * ax+by=e
   * cx+dy=f
   * where, x is iP[1] and y is iP[0]
   */
  double determinant = a*d - b*c; 
  if(determinant != 0) {
    iP[1] = (e*d - b*f)/determinant;
    iP[0] = (a*f - e*c)/determinant;
  } else {
    std::cout << "Cramer equations system: determinant is zero\n"
              "there are either no solutions or many solutions exist.\n";
    return false; 
  }
  return true;
}


void Linearizations::insertNewPt_(UInt j, UInt i, std::vector<double > & xc,
                             std::vector<double> & yc, ConstraintPtr newcon,
                             VariablePtr vl, VariablePtr vnl, bool & shouldCont)
{
  double f = newcon->getUb();
  LinearFunctionPtr lf = newcon->getLinearFunction();
  
  double d = lf->getWeight(vl);
  double c = lf->getWeight(vnl);
  double x1 = xc[j], y1 = yc[j], x2 = xc[i], y2 = yc[i], x, y;

  // point of intersection of newcon with the lin from j and j-1
  double a = y1-y2;
  double b = x2-x1;
  double e = y1*(x2-x1) - x1*(y2-y1);
  double determinant = a*d - b*c; 
  if(determinant != 0) {
    x = (e*d - b*f)/determinant;
    y = (a*f - e*c)/determinant;
    xc.insert(xc.begin()+j,x);
    yc.insert(yc.begin()+j,y);
  } else {
    std::cout << "Cramer equations system: determinant is zero\n"
              "there are either no solutions or many solutions exist.\n"; 
    shouldCont = false;
  }
}


//bool Linearizations::isFeas_(ConstSolutionPtr sol)
//{
  //int error=0;
  //double act, cUb;
  //ConstraintPtr c;
  //const double *x = sol->getPrimal();

  //for (CCIter it=nlCons_.begin(); it!=nlCons_.end(); ++it) {
    //c = *it;
    //act = c->getActivity(x, &error);
    //if (error == 0) {
      //cUb = c->getUb();
      //if ((act > cUb + solAbsTol_) &&
          //(cUb == 0 || act > cUb + fabs(cUb)*solRelTol_)) {
        //return false;
      //}
    //}	else {
      //logger_->msgStream(LogError) << me_ << c->getName() <<
        //" constraint not defined at this point."<< std::endl;
      //return false;
    //}
  //}
  //return true;
//}


void Linearizations::linearAt_(FunctionPtr f, double fval, const double *x,
                          double *c, LinearFunctionPtr *lf, int *error)
{
  int n = rel_->getNumVars();
  double *a = new double[n];
  VariableConstIterator vbeg = rel_->varsBegin(), vend = rel_->varsEnd();
  const double linCoeffTol =
    env_->getOptions()->findDouble("conCoeff_tol")->getValue();

  std::fill(a, a+n, 0.);
  f->evalGradient(x, a, error);
  
  if (*error==0) {
    *lf = (LinearFunctionPtr) new LinearFunction(a, vbeg, vend, linCoeffTol);
    *c  = fval - InnerProduct(x, a, minlp_->getNumVars());
  } else {
    logger_->msgStream(LogError) << me_ <<"gradient not defined at this point."
      << std::endl;
#if SPEW
    logger_->msgStream(LogDebug) << me_ <<"gradient not defined at this point."
      << std::endl;
#endif
  }
  delete [] a;
  return;
}


//MS: rootLinScheme3_ - individual cons - working
bool Linearizations::lineSearchPt_(double* x, const double* u,
                              ConstraintPtr con, double &nlpact)
{
  int error = 0;
  bool ptFound = true;
  double cUb = con->getUb();
  FunctionPtr f = con->getFunction();
  UInt numVars =  minlp_->getNumVars();
  
  double* xl = new double[numVars];
  double* xu = new double[numVars];

  std::copy(u, u+numVars, xu);
  std::copy(solC_, solC_+numVars, xl);
  
  while (true) {
    //MS: all vars or only in the nonlinear ones
    for (UInt i = 0 ; i < minlp_->getNumVars(); ++i) {
      x[i] = 0.5*(xl[i] + xu[i]);
    }
    nlpact = f->eval(x, &error);
    if (error == 0) {
      if (nlpact > cUb + solAbsTol_ && (cUb==0 ||
                                      nlpact > cUb + fabs(cUb)*solRelTol_)) {
        std::copy(x,x+numVars,xu);
      } else {
        if (fabs(cUb-nlpact) <= solAbsTol_ || 
            (cUb!=0 && fabs(cUb-nlpact) <= fabs(cUb)*solRelTol_)) {
          break;
        } else {
          std::copy(x,x+numVars,xl);
        }
      }
    } else {
      //MS: think later what can be done here
      ptFound = false;
      break;
    }
  }

  delete [] xl;
  delete [] xu;
  return ptFound;
}



void Linearizations::rootLinearizations(const double * nlpx)
{
  ConstraintPtr con;
  bool isFound = false;
  UInt nVarIdx, lVarIdx;
  double lVarCoeff = 0, nVarCoeff = 0;
    
  nlpx_ = nlpx;
 
  if (rs1_ || rs2Per_) { 
    for (CCIter it = nlCons_.begin(); it != nlCons_.end(); ++it) {
      con = *it;
      lVarIdx = 0; lVarCoeff = 0; nVarCoeff = 0;
      // constraints with only one var in the nonlinear part
      isFound = uniVarNlFunc_(con, lVarCoeff, lVarIdx, nVarIdx, nVarCoeff); 
      //MS: see if this if-else has to be changed
      if (isFound == false) {
        continue;    
      } else {
        if (rs1_ > 0) {
          rootLinScheme1_(con, lVarCoeff, lVarIdx, nVarIdx, nVarCoeff);
        }
        if (rs2Per_ > 0) { // there is a default neighborhood
          //nlpx_ = nlpx;
          rootLinScheme2_(con, lVarCoeff, lVarIdx, nVarIdx);  
        }
      }
    }
  }
  /// General scheme at root
  // Option for general scheme
  //isFound = false;
  if (rgs1_ || rgs2_) {
    //nlpe_->clear();
    //findCenter_();
    //findCenter_(isFound);
    if (solC_) {
      // populate varPtrs_ with index of variables in nonlinear constraints
      varsInNonlinCons_();
      // General scheme using center and  positive spanning vectors
      if (rgs1_) {
        rootLinGenScheme1_();
      }

      // General scheme using nonlinear solution and positive spanning vectors
      if (rgs2_) {
        rootLinGenScheme2_();
      }
    }
  }

  return;
}


void Linearizations::rootLinGenScheme2_()
{
  VariablePtr v;
  bool isFound = false;
  UInt fixIdx, vIdx, pos;
  std::vector<double > alphaSign;
  std::vector<UInt > varIdx; // first index is the coordinate direction componenet 
  int n = minlp_->getNumVars();
  double *xOut = new double[n], *lastDir = new double[n]; 
  double varbound, rhs = 0.0, coeff, fixCoeff;   
  
  std::copy(nlpx_, nlpx_+n, xOut);
    
  //// variable to be fixed in finding search direction
  for (UInt i = 0; i < varPtrs_.size(); ++i) {
    v = varPtrs_[i];
    vIdx = v->getIndex();
    rhs = rhs + nlpx_[vIdx]*(solC_[vIdx] - nlpx_[vIdx]);
    if (!isFound && (solC_[vIdx] - nlpx_[vIdx]) != 0) {
      fixIdx = vIdx;
      isFound = true;              
    }
  }
  
  //// in case rhs is 0, shift the plane to find search directions 
  if (rhs == 0) {
    rhs = 1;  
  }

  // coefficient of the fix var in any direction
  fixCoeff = rhs/(solC_[fixIdx] - nlpx_[fixIdx]);
  lastDir[fixIdx] = 0;
     
  // coordinate direction for each variable 
  for (UInt i = 0; i < varPtrs_.size(); ++i) {
    v = varPtrs_[i];
    vIdx = v->getIndex();
    if (vIdx < fixIdx) {
      // in this case coeff of v is zero in hyperplane expression 
      lastDir[vIdx] = -1;
      varbound = v->getUb();
      alphaSign.push_back(1);
      varIdx.push_back(vIdx);
      search_(varbound, vIdx, nlpx_[vIdx], varIdx, xOut, isFound, alphaSign,
              0, 0);
      //if (!isFound) {
        alphaSign[0] = -1;
        varbound = v->getLb();
        xOut[vIdx] = nlpx_[vIdx];
        search_(varbound, vIdx, nlpx_[vIdx], varIdx, xOut, isFound, alphaSign,
                0, 0);
      //}
    } else if (vIdx > fixIdx) {
      // in this case coeff of v is nonzero in the hyperplane expression 
      coeff = rhs/(solC_[vIdx] - nlpx_[vIdx]);
      lastDir[vIdx] = -coeff;
      lastDir[fixIdx] = lastDir[fixIdx] + fixCoeff;
      boundingVar_(varbound, vIdx, fixIdx, coeff, -fixCoeff,
                 alphaSign, varIdx);
      search_(varbound, varIdx[0], nlpx_[varIdx[0]], varIdx, xOut, isFound,
              alphaSign, 0, 0);
      //if (!isFound) {
        xOut[vIdx] = nlpx_[vIdx];
        xOut[fixIdx] = nlpx_[fixIdx];
        varIdx.clear();
        alphaSign.clear();
        boundingVar_(varbound, vIdx, fixIdx, -coeff, fixCoeff,
                 alphaSign, varIdx);
        search_(varbound, varIdx[0], nlpx_[varIdx[0]], varIdx, xOut, isFound, alphaSign, 0, 0);
      //}
      xOut[fixIdx] = nlpx_[fixIdx];
    }
    varIdx.clear();
    alphaSign.clear();
    xOut[vIdx] = nlpx_[vIdx];
  }

  // last direction in positive spanning set 
  boundingVar_(varbound, pos, lastDir, alphaSign);
  vIdx = varPtrs_[pos]->getIndex();
  search_(varbound, vIdx, nlpx_[vIdx] , varIdx, xOut, isFound, alphaSign, pos, 1);
  //if (!isFound) {
    for (UInt i = 0; i < varPtrs_.size(); ++i) {
      lastDir[i] = -lastDir[i];
    }
    std::copy(nlpx_, nlpx_+n, xOut);
    boundingVar_(varbound, pos, lastDir, alphaSign);
    vIdx = varPtrs_[pos]->getIndex();
    search_(varbound, vIdx, nlpx_[vIdx], varIdx, xOut, isFound, alphaSign, pos, 1);
  //}

  delete [] xOut;
  delete [] lastDir;
  return;
}


void Linearizations::search_(double varbound, UInt vIdx, double val, 
                             std::vector<UInt > varIdx, 
                             double *xOut, bool &isFound,
                             std::vector<double > &alphaSign,
                             UInt pos, bool isLastDir)
{
  double alpha;
  setStepSize_(varbound, alpha, vIdx, val, alphaSign[pos]);
  
  if (alpha == 0) {
    isFound = false;
    return;  
  }

  if (isLastDir) {
    UInt idx;
    for (UInt i = 0; i < varPtrs_.size(); ++i) {
      idx = varPtrs_[i]->getIndex();
      alphaSign[i] = alpha*alphaSign[i];  
      xOut[idx] = xOut[idx] + alphaSign[i];
    } 
  } else {
    for (UInt i = 0; i < alphaSign.size(); ++i) {
      alphaSign[i] = alpha*alphaSign[i];  
      xOut[varIdx[i]] = xOut[varIdx[i]] + alphaSign[i];
    }
  }
  isFound = foundLinPt_(vIdx, varIdx, pos, alphaSign, varbound, xOut, isLastDir);
  return;
}

// determine which out of varPtrs_ is bounding 
void Linearizations::boundingVar_(double &varbound, UInt &pos, 
                                  double *lastDir,
                                  std::vector<double > &alphaSign)
{
  UInt idx;
  double diff, minDiff = INFINITY, bound;
    
  for (UInt i = 0; i < varPtrs_.size(); ++i) {
    idx = varPtrs_[i]->getIndex();
    if (lastDir[i] < 0) {
      bound = varPtrs_[i]->getLb();
      diff = nlpx_[idx] - bound;
      alphaSign.push_back(-1);
    } else {
      bound = varPtrs_[i]->getUb();
      diff = bound - nlpx_[idx];
      alphaSign.push_back(1);
    }
    if (diff < minDiff) {
      pos = i;
      varbound = bound;    
    }
  }
  return;
}

// determine which out of vIdx and fixIdx is bounding 
void Linearizations::boundingVar_(double &varbound,
                                  UInt vIdx, UInt fixIdx, double coeff,
                                 double fixCoeff, 
                                 std::vector<double > &alphaSign, 
                                 std::vector<UInt > &varIdx)
{
  double diffCurrent, diffFix;
  VariablePtr v = varPtrs_[vIdx], fixVar = varPtrs_[fixIdx];

  if (coeff < 0) {
    diffCurrent = nlpx_[vIdx] - v->getLb();
    if (fixCoeff < 0) {
      diffFix = nlpx_[vIdx] - fixVar->getLb();
      if (diffCurrent < diffFix) {
        varIdx.push_back(vIdx); alphaSign.push_back(-1);
        varIdx.push_back(fixIdx); alphaSign.push_back(-1);
        varbound = v->getLb();
      } else {
        varIdx.push_back(fixIdx); alphaSign.push_back(-1);
        varIdx.push_back(vIdx); alphaSign.push_back(-1);
        varbound = fixVar->getLb();
      }
    } else {
      diffFix = fixVar->getUb() - nlpx_[vIdx];
      if (diffCurrent < diffFix) {
        varIdx.push_back(vIdx); alphaSign.push_back(-1);
        varIdx.push_back(fixIdx); alphaSign.push_back(1);
        varbound = v->getLb();
      } else {
        varIdx.push_back(fixIdx); alphaSign.push_back(1);
        varIdx.push_back(vIdx); alphaSign.push_back(-1);
        varbound = fixVar->getUb();
      }
    }
  } else {
    diffCurrent = v->getLb() - nlpx_[vIdx];
    if (fixCoeff < 0) {
      diffFix = nlpx_[vIdx] - fixVar->getLb();
      if (diffCurrent < diffFix) {
        varIdx.push_back(vIdx); alphaSign.push_back(1);
        varIdx.push_back(fixIdx); alphaSign.push_back(-1);
        varbound = v->getUb();
      } else {
        varIdx.push_back(fixIdx); alphaSign.push_back(-1);
        varIdx.push_back(vIdx); alphaSign.push_back(1);
        varbound = fixVar->getLb();
      }
    } else {
      diffFix = fixVar->getUb() - nlpx_[vIdx];
      if (diffCurrent < diffFix) {
        varIdx.push_back(vIdx); alphaSign.push_back(1);
        varIdx.push_back(fixIdx); alphaSign.push_back(1);
        varbound = v->getUb();
      } else {
        varIdx.push_back(fixIdx); alphaSign.push_back(1);
        varIdx.push_back(vIdx); alphaSign.push_back(1);
        varbound = fixVar->getUb();
      }
    } 
  }
  return;
}


void Linearizations::rootLinGenScheme1_()
{
  VariablePtr v;
  bool isFound = false;
  UInt lPos, uPos, vIdx;
  int n = minlp_->getNumVars();
  double *xOut = new double[n];
  double varbound, vLb = INFINITY, vUb = INFINITY;   
  std::vector<double > alphaSign;
  std::vector<UInt > varIdx; // first index is the coordinate direction componenet 

  std::copy(solC_, solC_ + n, xOut);

   // Take coordinate direction along each variable in varPtrs_.
   // If this direction is unsuccessful in finding a point to add 
   // linearization then reverse the direction.
  for (UInt i = 0; i < varPtrs_.size(); ++i) {
    v = varPtrs_[i];
    vIdx = v->getIndex();

    // determine indices of variables that are bounding in the last
    // direction (and its opposite) of search and bound values 
    // MS: think about this once again
    varbound = v->getUb() - solC_[vIdx];
    if (varbound < vUb) {
      uPos = i;
      vUb = varbound;
    }
    varbound = solC_[vIdx] - v->getLb();
    if (varbound < vLb) {
      lPos = i;
      vLb = varbound;
    }

     // coordinate direction for each variable 
    alphaSign.push_back(1);
    varIdx.push_back(vIdx);
    varbound = v->getUb();
    search_(varbound, vIdx, solC_[vIdx], varIdx, xOut, isFound, alphaSign, 0, 0);
    
     // reverse search direction if previous direction was unsuccessful 
    //if (!isFound) {
      alphaSign[0] = -1;
      varbound = v->getLb();
      xOut[vIdx] = solC_[vIdx];
      search_(varbound, vIdx, solC_[vIdx], varIdx, xOut, isFound, alphaSign, 0, 0);
    //}
    varIdx.clear();
    alphaSign.clear();
    xOut[vIdx] = solC_[vIdx];
  }
  
   // last direction in positive spanning set 
  if (vLb == INFINITY) {
    lPos = 0; 
    vIdx = varPtrs_[0]->getIndex(); 
  } else {
    vLb = varPtrs_[lPos]->getLb();
    vIdx = varPtrs_[lPos]->getIndex(); 
  }
  alphaSign.resize(varPtrs_.size());
  std::fill(alphaSign.begin(), alphaSign.end(),-1);
  
  search_(vLb, vIdx, solC_[vIdx], varIdx, xOut, isFound, alphaSign, lPos, 1);
 
  //if (!isFound) {
    if (vUb == INFINITY) {
      uPos = 0;  
      vIdx = varPtrs_[0]->getIndex(); 
    } else {
      vUb = varPtrs_[uPos]->getUb();
      vIdx = varPtrs_[uPos]->getIndex(); 
  }
    std::copy(solC_, solC_+minlp_->getNumVars(), xOut);
    std::fill(alphaSign.begin(), alphaSign.end(),1);
    search_(vUb, vIdx, solC_[vIdx], varIdx, xOut, isFound, alphaSign, uPos, 1);
  //}
  delete [] xOut;
  return;
}


void Linearizations::setStepSize_(double &varbound, double &alpha,
                                   UInt vIdx, double val,
                                   double boundSign)
{
  if (varbound != boundSign*INFINITY) {
    alpha = fabs(varbound-val);
  } else {
    if (fabs(nlpx_[vIdx] - solC_[vIdx]) != 0) {
      alpha = fabs(nlpx_[vIdx] - solC_[vIdx]);
    } else {
      alpha = fabs(val) + 4;
    }
    varbound = val + boundSign*(fabs(10*val) + 10); // parameter here
  }
  alpha = 0.25*alpha;
  return;
}
          

void Linearizations::varsInNonlinCons_()
{
  VariablePtr v;
  ConstraintPtr con;
  FunctionType type;
  QuadraticFunctionPtr qf;
  NonlinearFunctionPtr nlf;
  std::vector<VariablePtr>::iterator vit;

  for (ConstraintConstIterator c = minlp_->consBegin(); 
                  c != minlp_->consEnd(); ++c) {
    con = *c;
    type = con->getFunctionType();
    if (!(type == Linear || type == Constant)) {
      qf = con->getQuadraticFunction();
      nlf = con->getNonlinearFunction();
      
      if (nlf) {
        for (VarSetIter it=nlf->varsBegin(); it!=nlf->varsEnd(); ++it) {
          v = *it;
          vit = std::find (varPtrs_.begin(), varPtrs_.end(), v); 
          if (vit == varPtrs_.end()) {
            varPtrs_.push_back(v);
          }
        }
      }
      
      if (qf) {
        for (VariablePairGroupConstIterator it=qf->begin(); it!=qf->end(); 
             ++it) {
          v = it->first.first;
          vit = std::find (varPtrs_.begin(), varPtrs_.end(), v); 
          if (vit == varPtrs_.end()) {
            varPtrs_.push_back(v);
          }
        }
      }
    }
  }
  return;
}


bool Linearizations::foundLinPt_(UInt vIdx, std::vector<UInt> varIdx, 
                                 UInt pos,
                                 std::vector<double> alphaSign, double varbound,
                                 double *xOut, bool isLast)
{
  double act, cUb;
  ConstraintPtr con;
  FunctionType type;
  std::vector<ConstraintPtr > vioCons;
  bool isPtFound, shouldCont = false;
  int n = minlp_->getNumVars(), error = 0;
  
  /* find constraints violated at xOut. If no constraint is violated then
   * move further along the given direction, If all linear constraints are
   * violated then stop and return. 
   */
  while (true) {
    for (ConstraintConstIterator c=minlp_->consBegin(); 
                  c!=minlp_->consEnd(); ++c) {
      con = *c;
      cUb = con->getUb();
      type = con->getFunctionType();
      if (type == Constant) {
        continue;    
      } else {
        act = con->getActivity(xOut, &error);
        if (error == 0) {
          if ((act > cUb + solAbsTol_) &&
              (cUb == 0 || act > cUb + fabs(cUb)*solRelTol_)) { // violated cons
            vioCons.push_back(con);
            if (type != Linear) {
              shouldCont = 1;
            }
          }
        } else {
          return false;      
        }   
      }
    }
    if (vioCons.size() == 0) {
      newPoint_(isLast, varIdx, xOut, alphaSign);  
      //if (alphaSign[pos] < 0) {
        //if ((xOut[vIdx]-varbound) < 0) {
          //break;
        //}
      //} else {
        //if ((xOut[vIdx]-varbound) > 0) {
          //break;
        //}  
      //}
      if (alphaSign[pos]*(xOut[vIdx]-varbound) > 0) {
        break;
      }
    } else {
      break;    
    }
  }
  
  if (!shouldCont) { 
    /* all violated constraints are linear or violated by bound*/
    return false;  
  }

  /* find point on boundary along the direction */
  double *xIn = new double[n];
  std::copy(solC_, solC_+minlp_->getNumVars(), xIn);
  while (shouldCont) {
    shouldCont = findBoundaryPt_(isPtFound, xOut, xIn, vioCons);
  }
 
  delete [] xIn; 
  if (isPtFound) {
    return false;
  }
  return true;
}

void Linearizations::newPoint_(bool isLast,
                               std::vector<UInt> varIdx, double *xOut,
                               std::vector<double> alphaSign)
{
  UInt idx;
  if (isLast) {
    for (UInt i = 0; i < varPtrs_.size(); ++i) {
      idx = varPtrs_[i]->getIndex();
      xOut[idx] = xOut[idx] + alphaSign[i];
    }
  } else {
    for (UInt i = 0; i < varIdx.size(); ++i) {
      idx  = varIdx[i];
      xOut[idx] = xOut[idx] + alphaSign[i];
    }
  }
  return;
}

// MS: change return value here
bool Linearizations::findBoundaryPt_(bool &isPtFound,
                                     double *xOut, double *xIn,
                                     std::vector<ConstraintPtr> &vioCons)
{
  int error = 0;
  double act, cUb; 
  ConstraintPtr con;
  FunctionType type;
  std::vector<double > activeConsAct;
  UInt numVars =  minlp_->getNumVars();
  double* x = new double[numVars];
  bool vioNLCons = false, firstVio = false, firstActive = false, activeNLCons = false;
  
  isPtFound = false;
  for (UInt i = 0 ; i < minlp_->getNumVars(); ++i) {
    x[i] = 0.5*(xIn[i] + xOut[i]);
  }

  for (UInt k = 0; k < vioCons.size(); ) {
    con = vioCons[k];
    cUb = con->getUb();
    type = con->getFunctionType();
    if (type == Constant) {
      continue;    
    } else {
      act = con->getActivity(x, &error);
      if (error != 0) {
        delete [] x;
        return false;      
      }
      if ((act > cUb + solAbsTol_) &&
          (cUb == 0 || act > cUb + fabs(cUb)*solRelTol_)) { // violated
        if (!firstVio) {
          firstVio = true;
          if (k != 0) {
            vioCons.erase(vioCons.begin(),vioCons.begin() + k);
            k = 0;
          }
        }
        ++k;
        if (type != Linear) {
          vioNLCons = true; 
        }
      } else if ((fabs(act-cUb) <= solAbsTol_) ||
            (cUb != 0 && fabs(act- cUb) <= fabs(cUb)*solRelTol_)) { // active
        if (firstVio) {
          vioCons.erase(vioCons.begin() + k);
          --k;
        } else {
          if (!firstActive) {
            firstActive = true;         
            if (k != 0) {
              vioCons.erase(vioCons.begin(),vioCons.begin() + k);
              k = 0;
            }
          }
          activeConsAct.push_back(act);
          ++k;
          if (type != Linear) {
            activeNLCons = true; 
          }
        }
      } else {
         if (firstVio || firstActive) {
           vioCons.erase(vioCons.begin() + k);
           --k;
        } else {
           ++k;
        }   
      }
    }
  }

  if (!firstVio) {
    if (!firstActive) {
      /* all constraints are inactive. Find another point */
      std::copy(x,x+numVars,xIn);
    } else {
      if (activeNLCons) {
      /* add linearization to active nonlinear constraints */
        double c;
        UInt j = 0;
        FunctionPtr f;
        std::stringstream sstm;
        LinearFunctionPtr lf = 0;
        for (ConstraintIterator it = vioCons.begin();
             it != vioCons.end(); ++it, ++j) {
          con = *it;
          if (con->getFunctionType() == Linear) {
            continue;        
          }
          isPtFound = true;
          f = con->getFunction();
          linearAt_(f, activeConsAct[j], x, &c, &lf, &error);
          if (error == 0) {
            cUb = con->getUb();
            if (rgs1_) {
              ++(stats_->rgs1Cuts);
              sstm << "_OAcut_" << stats_->rgs1Cuts << "_AtRoot";
            } else if (rgs2_) {
              ++(stats_->rgs2Cuts);
              sstm << "_OAcut_" << stats_->rgs2Cuts << "_AtRoot";
            } else if (rs3_) {
              ++(stats_->rs3Cuts);
              sstm << "_OAcut_" << stats_->rs3Cuts << "_AtRoot";
            } else {
              // Later: print message here.            
            }
            f = (FunctionPtr) new Function(lf);
            rel_->newConstraint(f, -INFINITY, cUb-c, sstm.str());
            //newcon = rel_->newConstraint(f, -INFINITY, cUb-c, sstm.str());
            sstm.str("");
          }
        }
      }
      delete [] x;
      return false;
    }
  } else {
    if (vioNLCons) {
      std::copy(x,x+numVars,xOut);
    } else {
      delete [] x;
      return false;
    }
  } 
  delete [] x;
  return true;    
}
  

void Linearizations::rootLinScheme1_(ConstraintPtr con, double lVarCoeff,
                            UInt lVarIdx, UInt nVarIdx, double nVarCoeff)
{
  double iP[2]; // intersection point
  UInt newConId;
  bool shouldCont;
  ConstraintPtr newcon;
  std::vector<UInt > newConsId;
  VariablePtr vnl = NULL, vl = NULL;
  std::vector<double > linVioVal, xc, yc; // xc and yc  nonlinear and lin var
  int i, error = 0, n = rel_->getNumVars();
  double act, cUb, y1, y2, vLb, vUb, maxVio, stopCond, consUb; 
  double *b1 = new double[n];

  std::fill(b1, b1+n, 0.);
  vl = rel_->getVariable(lVarIdx);
  vnl = rel_->getVariable(nVarIdx);
  
  vLb = vnl->getLb();
  vUb = vnl->getUb();

  if (vLb == -INFINITY) {
    if (vUb == INFINITY) {
      vLb = -50;
      vUb = 50;
    } else {
      vLb = vUb - 100;
    }
  } else {
    if (vUb == INFINITY) {
      vUb = vLb + 100;
    } 
  }
    
  b1[nVarIdx] = vLb;
  act = nVarCoeff*vLb;
  shouldCont = linPart_(b1, lVarIdx, con, lVarCoeff, act);  
  if (shouldCont) {
    shouldCont = addNewCut_(b1, con, newConId);
    if (shouldCont) {
      y1 = b1[lVarIdx];
      newConsId.push_back(newConId); 
    } else {
      delete [] b1;
      return;    
    }
  } else {
    delete [] b1;
    return;    
  }

  // upper bound of var in nonlinear cons
  b1[nVarIdx] = vUb;
  act = nVarCoeff*vUb;
  shouldCont = linPart_(b1, lVarIdx, con, lVarCoeff, act);  
  if (shouldCont) {
    shouldCont = addNewCut_(b1, con, newConId);
    if (shouldCont) {
      y2 = b1[lVarIdx];
      newConsId.push_back(newConId); 
    } else {
      delete [] b1;
      return;    
    }
  } else {
    delete [] b1;
    return;    
  }

  shouldCont = findIntersectPt_(newConsId, vl, vnl, iP);
  if (shouldCont == false) {
    delete [] b1;
    return;    
  }

  // populate points and their violation in cyclic order
  xc.push_back(vLb);
  yc.push_back(y1);
  linVioVal.push_back(0);
      
  b1[nVarIdx] = iP[0], b1[lVarIdx] = iP[1];
  act = con->getActivity(b1, &error);
  if (error != 0) {
    delete [] b1;
    return;    
  }  
  consUb = con->getUb();
  act = std::max(act-consUb, 0.0); 
  xc.push_back(iP[0]), yc.push_back(iP[1]), linVioVal.push_back(act);
  
  xc.push_back(vUb), yc.push_back(y2), linVioVal.push_back(0);

  // starting from intersection point
  i = 1;
  maxVio = linVioVal[i];
  if (fabs(consUb) > solAbsTol_) { 
    stopCond = consUb*rs1_/100;    
  } else {
    stopCond = maxVio*rs1_/100;    
  }

  if ((stopCond < solAbsTol_) || 
      (consUb!=0 && stopCond < fabs(consUb)*solRelTol_ )) { 
    delete [] b1;
    return;
  }

  while (maxVio >= stopCond) { 
    //add a new cut at the point indexed i
    b1[nVarIdx] = xc[i];
    shouldCont = addNewCut_(b1, con, newConId);
    if (shouldCont) {
      newcon = rel_->getConstraint(newConId);
      cUb = newcon->getUb();
    } else {
      break;
    }
    
    // Move right and determine first point that satisfy the newcon
    for (UInt j = i+1; j < xc.size(); ) {
      b1[nVarIdx] = xc[j], b1[lVarIdx] = yc[j];
      act = newcon->getActivity(b1, &error);
      if (error == 0) {
        if ((act < cUb + solAbsTol_) ||
            (cUb =! 0 && act < cUb + fabs(cUb)*solRelTol_)) {
            //insert new point just before index j
          insertNewPt_(j, j-1, xc, yc, newcon, vl, vnl, shouldCont); 
          b1[nVarIdx] = xc[j], b1[lVarIdx] = yc[j];
          act = con->getActivity(b1, &error);
          if (error != 0) {
            shouldCont = false;    
          } else {
            act = std::max(act-consUb, 0.0); 
            linVioVal.insert(linVioVal.begin()+j,act);
          }
          break;
        } else {
          // delete point if violates newcon
          xc.erase(xc.begin() + j);
          yc.erase(yc.begin() + j);        
          linVioVal.erase(linVioVal.begin() + j);        
        }  
      }
    }
    if (shouldCont == false) {
      break;
    }
    int j = i-1;
    while (j >= 0) {
      b1[nVarIdx] = xc[j], b1[lVarIdx] = yc[j];
      act = newcon->getActivity(b1, &error);
      if (error == 0) {
        if ((act < cUb + solAbsTol_) ||
            (cUb =! 0 && act < cUb + fabs(cUb)*solRelTol_)) {
          insertNewPt_(j+1, j, xc, yc, newcon, vl, vnl, shouldCont); 
          b1[nVarIdx] = xc[j+1], b1[lVarIdx] = yc[j+1];
          act = con->getActivity(b1, &error);
          if (error != 0) {
            shouldCont = false;    
          } else {
            act = std::max(act-consUb, 0.0); 
            linVioVal.insert(linVioVal.begin()+j+1,act);
            xc.erase(xc.begin()+j+2);
            yc.erase(yc.begin()+j+2);        
            linVioVal.erase(linVioVal.begin()+j+2);        
          }
          break;
        }  else {
          // delete point if violates newcon
          xc.erase(xc.begin() + j);
          yc.erase(yc.begin() + j);        
          linVioVal.erase(linVioVal.begin() + j);        
          --j;
        }  
      }
    }
    if (shouldCont == false) {
      break;
    }
    maxVio = *(std::max_element(linVioVal.begin(), linVioVal.end()));
    if ((maxVio < solAbsTol_) || 
        (consUb!=0 && maxVio < fabs(consUb)*solRelTol_ )) { 
      break;
    }
    i = std::max_element(linVioVal.begin(), linVioVal.end())-linVioVal.begin();     
  }
  delete [] b1;
  return;
}


void Linearizations::rootLinScheme2_(ConstraintPtr con,
                                     double lVarCoeff,
                                     UInt lVarIdx, UInt nVarIdx)
{
  int error = 0;
  FunctionPtr f;
  VariablePtr vnl;
  //bool isCont = true;
  UInt n = minlp_->getNumVars();
  double lastSlope, delta, nlpSlope, nbhSize;
  
  vnl = rel_->getVariable(nVarIdx);
  
  double* npt = new double[n];
  std::fill(npt, npt+n, 0.);
  
  double *grad = new double[n];
  std::fill(grad, grad+n, 0.);
  
  f = con->getFunction();
  f->evalGradient(nlpx_, grad, &error);
  
  nlpSlope = -1*(grad[nVarIdx]/lVarCoeff);
  lastSlope = nlpSlope;
  
  nbhSize = std::max(vnl->getLb(), nlpx_[nVarIdx] - rs2NbhSize_);    
  if (nlpx_[nVarIdx] - nbhSize >= 1) {
    delta = 0.5;  
  } else {
    delta = nlpx_[nVarIdx] - nbhSize;  
  }

  npt[nVarIdx] = nlpx_[nVarIdx] - delta;
     
  if (delta != 0) {
    while (npt[nVarIdx] >= nbhSize) {
      grad[nVarIdx] = 0;
      grad[lVarIdx] = 0;
      rScheme2Cut_(con, delta, lVarCoeff, lastSlope, nVarIdx, npt, grad);
      npt[nVarIdx] =  npt[nVarIdx] - delta;
    }
  }
  
  nbhSize = std::min(vnl->getUb(), nlpx_[nVarIdx] + rs2NbhSize_);
  if (nbhSize - nlpx_[nVarIdx] >= 1) {
    delta = 0.5;  //MS: can be a parameter  
  } else {
    delta = nbhSize - nlpx_[nVarIdx];  
  }

  lastSlope = nlpSlope;
  npt[nVarIdx] = nlpx_[nVarIdx] + delta;

  if (delta != 0) {
    while (npt[nVarIdx] <= nbhSize) {
      grad[nVarIdx] = 0;
      grad[lVarIdx] = 0;
      rScheme2Cut_(con, delta, lVarCoeff, lastSlope, nVarIdx, npt, grad);
      npt[nVarIdx] =  npt[nVarIdx] + delta;
    }
  }
  delete [] grad;
  delete [] npt;
  return;
}


void Linearizations::rScheme2Cut_(ConstraintPtr con, double &delta,
                                double lVarCoeff, double &lastSlope,
                                UInt nVarIdx, double * npt, double * grad)
{
  int error = 0;
  double newSlope;
  FunctionPtr f = con->getFunction();
  f->evalGradient(npt, grad, &error);
  if (error != 0) {
    return;
  } 
  newSlope = -1*(grad[nVarIdx]/lVarCoeff); 
  if ((lastSlope == 0 && newSlope == 0) ||
      (lastSlope != 0 && fabs((newSlope-lastSlope)/lastSlope)*100 <  rs2Per_)) {
    delta = 2*delta;
    return;
  }
  lastSlope = newSlope;
  //ConstraintPtr newcon;
  std::stringstream sstm;
  LinearFunctionPtr lf = 0;
  VariableConstIterator vbeg = rel_->varsBegin(), vend = rel_->varsEnd();
  const double linCoeffTol =
    env_->getOptions()->findDouble("conCoeff_tol")->getValue();
  double c, cUb = con->getUb(), act = con->getActivity(npt, &error);
  
  lf = (LinearFunctionPtr) new LinearFunction(grad, vbeg, vend, linCoeffTol);
  c  = act - InnerProduct(npt, grad, minlp_->getNumVars());

  ++stats_->rs2Cuts;
  sstm << "_OAcut_" << stats_->rs2Cuts << "_AtRoot";
  f = (FunctionPtr) new Function(lf);
  rel_->newConstraint(f, -INFINITY, cUb-c, sstm.str());
  //newcon = rel_->newConstraint(f, -INFINITY, cUb-c, sstm.str());
  //newcon->write(std::cout);
  return;
}


bool Linearizations::shouldStop_(EngineStatus eStatus)
{
  bool shouldStop = false;
  switch (eStatus) {
   case (FailedInfeas):
     logger_->msgStream(LogInfo) << me_ << "failed to converge "
     << "(infeasible) in root" << std::endl;
     shouldStop = true;
     break;
   case (ProvenFailedCQInfeas):
     logger_->msgStream(LogInfo) << me_ << "constraint qualification "
                                        << "violated in root "
                                        << std::endl;
   case (ProvenInfeasible):
   case (ProvenLocalInfeasible):
     shouldStop = true;
     break;
   case (ProvenObjectiveCutOff):
     shouldStop = true;
     break;
   case (ProvenUnbounded):
     shouldStop = false;
     logger_->msgStream(LogDebug2) << me_ << "problem relaxation is "
                                   << "unbounded!" << std::endl;
     assert(!"Relaxation unbounded."); 
     break;
   case (FailedFeas):
     logger_->msgStream(LogInfo) << me_ << "Failed to converge " 
                                 << "(feasible) in root " << std::endl;
     break;
   case (ProvenFailedCQFeas):
     logger_->msgStream(LogInfo) << me_ << "constraint qualification "
                                 << "violated in root" << std::endl;
     break;
   case (EngineIterationLimit):
     logger_->msgStream(LogInfo) << me_ << "engine hit iteration limit, "
                                 << "continuing in root" << std::endl;
     // continue with this node by following ProvenLocalOptimal case.
   case (ProvenLocalOptimal):
   case (ProvenOptimal):
     break;
   case (EngineError):
     shouldStop = true;
     break;
   default:
     break;
  }
  return shouldStop;
}

//MS: add esh all from LP solution - working
void Linearizations::rootLinScheme3(EnginePtr lpe, VariablePtr objVar,
                                    SeparationStatus *status)
{
 //// ESH to all nonlinear constraints 
  int error = 0;
  const double *lpx;
  ConstraintPtr con;
  bool isFound, oNl = false;
  ObjectivePtr o = minlp_->getObjective();
  double act, cUb, objVal, minDist = INFINITY;
  UInt numOldCuts, numVars = minlp_->getNumVars();
  double *x = new double[numVars];
  double *boundaryPt = new double[numVars];
  
  if (o) {
    objVal = lpe->getSolution()->getObjValue();
    FunctionType fType = o->getFunctionType();
    if (fType != Linear && fType != Constant) {
      oNl = true;
    }
  }
  for (UInt i = 1; i <= rs3_; ++i) {
    numOldCuts = stats_->rs3Cuts;
    lpx = lpe->getSolution()->getPrimal();
    lpe->getSolution()->writePrimal(std::cout);
    std::cout << lpe->getSolution()->getObjValue() << std::endl;
    for (CCIter it = nlCons_.begin(); it!=nlCons_.end(); ++it) {
      con = *it;
      cUb = con->getUb();
      act = con->getActivity(lpx, &error);
      if (error == 0) {
        if ((act > cUb + solAbsTol_) &&
            (cUb == 0 || act > cUb + fabs(cUb)*solRelTol_)) {
          isFound = cutAtLineSearchPt_(lpx, x, con);
          if (oNl && isFound) {
            act = getDistance(solC_, x, numVars);
            if (act < minDist) {
              std::copy(x, x+numVars, boundaryPt);          
            }
          }
        }
      } else {
        logger_->msgStream(LogError) << me_ << "Constraint" <<  con->getName()
          << " is not defined at this point." << std::endl;
      }
    }
    if (numOldCuts < stats_->rs3Cuts) {
      // linearize objective at boundaryPt
      if (oNl) {
        error = 0;
        FunctionPtr f;
        double c, lpvio;
        LinearFunctionPtr lf;
        //ConstraintPtr newcon;
        std::stringstream sstm;
        act = o->eval(boundaryPt, &error);

        if (error == 0) {
          lpvio = std::max(act-objVal, 0.0);
          if ((lpvio > solAbsTol_) &&
              (objVal == 0 || lpvio > fabs(objVal)*solRelTol_)) {
              lf = 0;
              f = o->getFunction();
              linearAt_(f, act, boundaryPt, &c, &lf, &error);
              if (error == 0) {
                lpvio = std::max(c+lf->eval(boundaryPt)-objVal, 0.0);
                if ((lpvio > solAbsTol_) && ((objVal-c) == 0
                                           || lpvio > fabs(objVal-c)*solRelTol_)) {
                  ++(stats_->rs3Cuts);
                  lf->addTerm(objVar, -1.0);
                  f = (FunctionPtr) new Function(lf);
                  sstm << "_OAObjCut_" << stats_->rs3Cuts;
                  rel_->newConstraint(f, -INFINITY, -1.0*c, sstm.str());
                  //newcon = rel_->newConstraint(f, -INFINITY, -1.0*c, sstm.str());
                } else {
                  delete lf;
                  lf = 0;
                }
              }
            }
        }	else {
          logger_->msgStream(LogError) << me_
            << " objective not defined at this solution point." << std::endl;
        }
      }

      lpe->solve();
      isFound = shouldStop_(lpe->getStatus());
      if (isFound) {
        break;    
      }
    } else {
      break;
    }
  }

  if (stats_->rs3Cuts > 0) {
    *status = SepaResolve;
  }
  delete [] x;
  return;


  //// ESH only at the boundary point
  //double rootLpVal = lpe->getSolution()->getObjValue(), lpVal; 
  //bool shouldCont, isPtFound = false;
  //std::vector<ConstraintPtr > vioCons;
  //UInt numOldCuts, numVars = minlp_->getNumVars();
  //const double *lpx = lpe->getSolution()->getPrimal();
  //double *x = new double[numVars];
  //std::copy(lpx, lpx+numVars, x);          
  
  //for (UInt i = 1; i <= rs3_; ++i) {
    //for (CCIter it=nlCons_.begin(); it!=nlCons_.end(); ++it) {
      //vioCons.push_back(*it);
    //}
    //shouldCont = true;
    //numOldCuts = stats_->rs3Cuts;
    ////MS: shouldCont is confusing here
    //while (shouldCont) {
      //shouldCont = findBoundaryPt_(isPtFound, x, solC_, vioCons);
    //}
    //if (numOldCuts < stats_->rs3Cuts) {
      //lpe->solve();
      //shouldCont = shouldStop_(lpe->getStatus());
      //if (shouldCont) {
        //break;    
      //}
      ////lpVal = lpe->getSolution()->getObjValue();
      ////if (lpVal < (rs3_+100)*rootLpVal/100) {
        ////break;      
      ////}
    //} else {
      //break;
    //}
    //vioCons.clear();
  //}

  //if (stats_->rs3Cuts > 0) {
    //*status = SepaResolve;
  //}
  
  //delete [] x;
  //return;
}


bool Linearizations::uniVarNlFunc_(ConstraintPtr con, double &lVarCoeff,
                            UInt & lVarIdx, UInt & nVarIdx, double &nVarCoeff)
{
  double coeff;
  bool foundVar = false, foundNVar = false;
  UInt nlTerms = 0, qTerms = 0, idx;
  LinearFunctionPtr lf = con->getLinearFunction();
  QuadraticFunctionPtr qf = con->getQuadraticFunction();
  NonlinearFunctionPtr nlf = con->getNonlinearFunction();
  const double linCoeffTol =
    env_->getOptions()->findDouble("conCoeff_tol")->getValue();
  if (nlf) {
    nlTerms = nlf->numVars();
    if (nlTerms != 1) {
      return false;    
    }
    nVarIdx= (*(nlf->varsBegin()))->getIndex(); //index of var in nonlinear term
  }

  if (qf) {
    qTerms = qf->getNumVars();
    if (qTerms != 0) {
      if (qTerms > 1) {
        return false;    
      }
      if (nlTerms > 1) {
        if (nVarIdx != ((qf->varsBegin())->first)->getIndex()) {
          return false;      
        }
      } else {
        nVarIdx = ((qf->varsBegin())->first)->getIndex(); 
      }
    }
  }
  
  if (lf) {
    for(VariableGroupConstIterator vit = lf->termsBegin();
        vit != lf->termsEnd(); ++vit) {
      coeff = vit->second;
      idx = (vit->first)->getIndex();
      if (idx == nVarIdx) {
        foundNVar = true;
        nVarCoeff = coeff;
        continue;      
      }
      if (fabs(coeff) > linCoeffTol && foundVar == false) {
        lVarIdx = idx;
        foundVar = true;
        lVarCoeff = coeff;
      }
      if (foundVar && foundNVar) {
        break;      
      }
    }
  }
  
  if (foundVar) {
    return true;
  }
  return false;
}


double Linearizations::maxVio(const double *x, int &index)
{
  ConstraintPtr c;
  int error=0, i = 0;
  double act, cUb, vio = 0.0, max = -INFINITY;

  for (CCIter it=nlCons_.begin(); it!=nlCons_.end(); ++it, ++i) {
    c = *it;
    act = c->getActivity(x, &error);
    if (error == 0) {
      cUb = c->getUb();
      if (act > cUb+solAbsTol_ && (cUb == 0 ||
                                   act > cUb + fabs(cUb)*solRelTol_)) {
        //if (fabs(cUb) > solAbsTol_ && fabs(cUb) > solRelTol_) {
        if (fabs(cUb) > solAbsTol_) {
          vio = 100*(act - cUb)/fabs(cUb);      
        } else {
          vio = act - cUb;
        }
        if (vio > max) {
          max = vio;          
          index = i; 
        }
      }      
    }
  }
  return max;
}


void Linearizations::writeStats(std::ostream &out) const
{
  out
    << me_ << "number of cuts in root scheme 1      = "
    << stats_->rs1Cuts << std::endl
    << me_ << "number of cuts in root scheme 2      = "
    << stats_->rs2Cuts << std::endl
    << me_ << "number of cuts in root scheme 3      = "
    << stats_->rs3Cuts << std::endl
    << me_ << "number of cuts in root gen. scheme 1 = "
    << stats_->rgs1Cuts << std::endl
    << me_ << "number of cuts in root gen. scheme 2 = "
    << stats_->rgs2Cuts << std::endl;
  return;
}


// Local Variables:
// mode: c++
// eval: (c-set-style "k&r")
// eval: (c-set-offset 'innamespace 0)
// eval: (setq c-basic-offset 2)
// eval: (setq fill-column 78)
// eval: (auto-fill-mode 1)
// eval: (setq column-number-mode 1)
// eval: (setq indent-tabs-mode nil) 
// End:
