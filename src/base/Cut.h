//
//     MINOTAUR -- It's only 1/2 bull
//
//     (C)opyright 2010 - 2024 The Minotaur Team.
//

/**
 * \file Cut.h
 * \brief Declare the Cut class of valid inequalities
 * \author Ashutosh Mahajan, Argonne National Laboratory
 */

#ifndef MINOTAURCUT_H
#define MINOTAURCUT_H

#include "Types.h"

namespace Minotaur {
  
  class Function;

  struct CutInfo {
    UInt timesEnabled;       /// No. of times it was moved from pool to problem. 
    UInt timesDisabled;      /// No. of times it was removed from problem.
    UInt lastEnabled;        /// How many iters since it was last enabled.
    UInt lastDisabled;       /// How many iters since it was last disabled.
    UInt cntSinceActive;     /// Updated for cuts enabled in problem.
    UInt cntSinceViol;	     /// Updated for cuts disabled in pool.
    UInt numActive;	     /// Updated only for cuts in problem.
    int parent_active_cnts;  /// No. of cuts active in a node with
                             /// un-processed children

    double hash;             /// Hash value of this cut.
    double varScore;         /// Variable score (changes every iteration.)
    double fixedScore;       /// Fixed score (does not change.)

    bool neverDelete;        /// If true, never delete cut from pool.
    bool neverDisable;       /// If true, never remove cut from problem.
    bool inRel;		     /// Whether the cut is in Rel or Pool
  };


  /**
   * \brief Store function, bounds and other information about a cut.
   *
   * The Cut class is meant to store a cut generated by
   * different cut generators and handlers. This is a base class and special
   * classes of cuts can be derived from it. Also stores auxiliary information
   * and statistics about this cut.
   */
  class Cut { 

  public:
    /// Empty Constructor.
    Cut();

    /**
     * \brief Default constructor.
     *
     * \param [in] n Number of variables in the problem to which this cut is
     * applied. Used for evaluating a hash value.
     * \param [in] f  Function \f$f\f$ in the cut \f$l \leq f() \leq u\f$.
     * \param [in] lb Lower bound \f$l\f$ in the above cut.
     * \param [in] ub Upper bound \f$u\f$ in the above cut.
     * \param [in] never_delete If true, this cut is never deleted from the
     * pool.
     * \param [in] never_disable If true, this cut is never removed from the
     * problem.
     */
    Cut(UInt n, FunctionPtr f, double lb, double ub, bool never_delete,
        bool never_disable);


    Cut(ProblemPtr p, FunctionPtr f, double lb, double ub, bool never_delete,
        bool never_disable);
    /// Destroy.
    ~Cut();

    /**
     * \brief Add a cut to the problem.
     * \param [in] p The given problem.
     */
    void applyToProblem(ProblemPtr p);

    /**
     * \brief Evaluate the activity of this cut at a given point.
     * \param [in] x The given point.
     * \param [out] err Zero if no error ocurred in evaluation.
     * \return The activity at the given point.
     */
    double eval(const double *x, int *err);

    /**
     * \brief Evaluate score of this cut at a given point
     * \param [in] x The given point.
     * \param [out] vio Violation of this cut.
     * \param [out] score Score if this cut.
     */
    void evalScore(const double *x, double *vio, double *score);

    /**
     * \brief Get Constraint pointer if this cut is in the problem. Null
     * otherwise.
     */
    ConstraintPtr getConstraint() {return cons_;}

    /// Get Function Pointer of the cut.
    FunctionPtr getFunction() { return f_; }

    /// Get pointer to the cut info data structure.
    CutInfo * getInfo() {return &info_;}

    /// Get lb of the inequality.
    double getLb() { return lb_; }

    /// Get name of the cut
    std::string getName() { return name_; }

    /// Get ub of the inequality.
    double getUb() { return ub_; }

    /// Constraint associated with the cut
    void setCons(ConstraintPtr c) { cons_ = c; }

    /// Set name of the cut
    void setName_(std::string name) { name_ = name; }

    /// Display.
    void write(std::ostream &out) const;

    /// Display statistics and information.
    void writeStats(std::ostream &out) const;

  protected:
    /// Pointer to the constraint. Null if it is disabled.
    ConstraintPtr cons_;

    /// Pointer to the function of a cut.
    FunctionPtr f_;

    /// Information about the cut
    CutInfo info_;

    /// Lower bound.
    double lb_;

    /// Logger for display.
    LoggerPtr logger_;

    /// Number of variables in the problem. Used to calculate hash.
    UInt n_;

    /// Upper bound
    double ub_;

    /// Fixed score of each cut.
    double fixedScore_;

    /// Name of the cut (could be NULL).
    std::string name_;

    /**
     * \brief Initialize the values in info_ data structure.
     * \param [in] never_delete True if cut should never leave the pool.
     * \param [in] never_disable True if cut should never be removed from
     * the problem.
     */
    void initInfo_(bool never_delete, bool never_disable);

    /**
     * \brief Assign a fixed score to the cut
     */
    void evalFixedScore_();
  };

  typedef std::vector< CutPtr > CutVector;
  typedef CutVector::iterator CutIterator;
  typedef CutVector::const_iterator CutConstIterator;
}
#endif

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
