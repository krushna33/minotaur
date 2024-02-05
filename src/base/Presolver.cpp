// 
//     Minotaur -- It's only 1/2 bull
// 
//     (C)opyright 2008 - 2024 The Minotaur Team.
// 
/**
 * \file Presolver.cpp
 * \brief Define Presolver class for presolving.
 * \author Ashutosh Mahajan, Argonne National Laboratory
 */

#include <cmath>

#include "MinotaurConfig.h"
#include "Constraint.h"
#include "Environment.h"
#include "Function.h"
#include "Handler.h"
#include "Logger.h"
#include "LinearFunction.h"
#include "QuadraticFunction.h"
#include "Objective.h"
#include "Option.h"
#include "PreMod.h"
#include "Presolver.h"
#include "Problem.h"
#include "Solution.h"
#include "Variable.h"

using namespace Minotaur;

const std::string Presolver::me_ = "Presolver: ";

Presolver::Presolver ()
 : env_(0),
   eTol_(1e-8),
   handlers_(0),
   intTol_(1e-6),
   logger_(0),
   problem_(ProblemPtr()),
   sol_(0),
   status_(NotStarted)
{
}


Presolver::Presolver(ProblemPtr problem, EnvPtr env, HandlerVector handlers)
  : eTol_(1e-8),
    handlers_(handlers),
    intTol_(1e-6),
    problem_(problem),
    sol_(0),
    status_(NotStarted)
{
  env_ = env;
  logger_ = env->getLogger();
}


Presolver::~Presolver()
{
  handlers_.clear();
  for (PreModQIter m=mods_.begin(); m!=mods_.end(); ++m) {
    delete (*m);
  }
  mods_.clear();
  if (sol_) {
    delete sol_;
  }
}


SolveStatus Presolver::getStatus()
{
  return status_;
}


void Presolver::standardize()
{
  minimizify_();
  //if (problem_->isQuadratic() || problem_->isLinear()) { //MS: needed to do
  //even for a nonlinear problem
    ifIntsAreBins_();
    standardizeConstraints_();
    problem_->calculateSize(true);
  //}
}


SolveStatus Presolver::solve()
{
  SolveStatus h_status;
  bool changed = true;
  bool stop = false;
  status_ = Started;
  int iters = 0;
  int subiters = 0;
  int n_hand = handlers_.size();
  int last_ch_subiter = -10000;


  env_->getLogger()->msgStream(LogInfo) << me_ << "Presolving ... "
    << std::endl;
  // call all handlers.
  while (true==changed && false==stop && iters<5) {
    logger_->msgStream(LogDebug) << me_ << "major iteration " << iters << std::endl;
    for (HandlerIterator h = handlers_.begin(); h != handlers_.end(); ++h) {
      changed = false;
      h_status = (*h)->presolve(&mods_, &changed, &sol_);
      if (h_status == SolvedOptimal) {
        logger_->msgStream(LogDebug) << me_ << "handler " << (*h)->getName()
                                     << " found an optimal solution "
                                     << std::endl;
        status_ = SolvedOptimal;
        stop = true;
        if (!sol_) {
          logger_->errStream() << me_ << " but " << (*h)->getName()
                                      << " did not return a solution"
                                      << std::endl;
          status_ = SolveError;
        }
        break;
      } else if (h_status == SolvedInfeasible || h_status == SolvedUnbounded) {
        status_ = h_status;
        stop = true;
        break;
      }
      if (changed) {
        last_ch_subiter = subiters;
      }
      if (subiters>n_hand-2 && subiters-last_ch_subiter>n_hand-2) {
        stop = true;
        break;
      }
      ++subiters;
    }
    ++iters;
  }
  if (Started == status_) {
    status_ = Finished;
  }

  // wrap up.
  env_->getLogger()->msgStream(LogInfo) << me_ << "Finished presolving."
    << std::endl;
  for (HandlerVector::iterator it=handlers_.begin(); it!=handlers_.end();
       ++it) {
    (*it)->writeStats(logger_->msgStream(LogExtraInfo));
  }
  problem_->calculateSize(true);

  logger_->msgStream(LogDebug) << me_ << "Modifying debug solution."
    << std::endl;
  if (mods_.size()>0) {
    logger_->msgStream(LogExtraInfo) << me_
      << "ERROR: code to modify debug sol after presolve not available"
      << std::endl;
  }

  problem_->isDebugSolFeas(env_->getOptions()-> findDouble("feasAbs_tol")->
                           getValue(),
                           env_->getOptions()->findDouble("feasRel_tol")->
                           getValue());
  
  if (true == env_->getOptions()->findBool("display_presolved_size")->
      getValue()) {
    problem_->writeSize(logger_->msgStream(LogNone));
  }
  if (true == env_->getOptions()->findBool("display_presolved_problem")->
      getValue()) {
    problem_->write(logger_->msgStream(LogNone));
  }

if (env_->getOptions()->findBool("display_presolved_size")->getValue()==true) {
    env_->getLogger()->msgStream(LogInfo) << me_ << "Starting constraint classification\n";
    problem_->classifyCon(false);
    env_->getLogger()->msgStream(LogInfo) << me_ << "Finished constraint classification\n";
  }

  return status_;
}


void Presolver::removeEmptyObj_()
{
  ObjectivePtr oPtr = problem_->getObjective();
  if (oPtr && !(oPtr->getLinearFunction()) && !(oPtr->getQuadraticFunction()) &&
      !(oPtr->getNonlinearFunction())) {
    problem_->removeObjective();
  }
}


void Presolver::minimizify_()
{
  ObjectivePtr oPtr = problem_->getObjective();
  if (oPtr) {
    if (oPtr->getObjectiveType() == Maximize) {
      problem_->negateObj();
    }
  } 
}


// This function should be called only after minimizify_()
void Presolver::linearizeObjective_()
{
  assert(problem_);
  ObjectivePtr oPtr = problem_->getObjective();

  if (oPtr) {
    assert (!(oPtr->getFunctionType() == Nonlinear));
    if (oPtr->getFunctionType() == Quadratic) {
      // add a new variable
      std::string name = "obj_dummy_var";
      VariablePtr vPtr = problem_->newVariable(-INFINITY, INFINITY, 
          Continuous, name); 

      // add this variable to the objective
      LinearFunctionPtr lf = (LinearFunctionPtr) new LinearFunction();
      lf->addTerm(vPtr, 1.0);
      problem_->addToObj(lf);

      // remove quadratic parts from the objective
      QuadraticFunctionPtr qf = problem_->removeQuadFromObj();

      // add a new constraint containing the new variable and the quadratic.
      // qf - lf <= 0
      lf->multiply(-1.0);
      FunctionPtr f = (FunctionPtr) new Function(lf, qf);
      problem_->newConstraint(f, -INFINITY, 0.0);
    }
  } 
}


void Presolver::ifIntsAreBins_()
{
  VariablePtr v_ptr;
  double lb, ub;
  for (VariableConstIterator v_iter=problem_->varsBegin(); 
      v_iter!=problem_->varsEnd(); ++v_iter) {
    v_ptr = *v_iter;
    if (v_ptr->getType()==Integer) {
      lb = v_ptr->getLb();
      ub = v_ptr->getUb();
      if (ub <= 1.0 + intTol_ && lb >= -intTol_) {
        problem_->setVarType(v_ptr, Binary);
      }
    }
  }
}


void Presolver::standardizeConstraints_()
{
  ConstraintPtr c_ptr;
  for (ConstraintConstIterator c_iter=problem_->consBegin(); 
       c_iter!=problem_->consEnd(); ++c_iter) {
    c_ptr = *c_iter;
    if (c_ptr->getLb() > -INFINITY && c_ptr->getUb() >= INFINITY) {
      problem_->reverseSense(c_ptr);
    }
  }
}


void Presolver::getX(const double *x, DoubleVector *newx)
{
  assert(newx);

  if (mods_.size()>0) {
    DoubleVector xx(problem_->getNumVars());
    std::copy(x, x+problem_->getNumVars(), xx.begin());
    // call all mods.
    for (PreModQIter m=mods_.begin(); m!=mods_.end(); ++m) {
      (*m)->postsolveGetX(xx, newx);
      xx = *newx; // make a copy
    }
  } else {
    newx->resize(problem_->getNumVars());
    std::copy(x, x+problem_->getNumVars(), newx->begin());
  }
}


SolutionPtr Presolver::getPostSol(SolutionPtr s)
{
  DoubleVector  *newx = 0;
  SolutionPtr news = SolutionPtr(); // NULL
  if (s) {
    newx = new DoubleVector();
    getX(s->getPrimal(), newx);
    news = (SolutionPtr) new Solution(s->getObjValue(), *newx, problem_); 
    delete newx;
  }
  return news;
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
