/* 
 *     MINOTAUR -- It's only 1/2 bull
 *
 *     (C)opyright 2009 - 2024 The Minotaur Team.
 */

#ifndef LOGGERUT_H
#define LOGGERUT_H

#include <string>

#include <cppunit/TestCase.h>
#include <cppunit/TestCaller.h>
#include <cppunit/TestSuite.h>
#include <cppunit/TestResult.h>
#include <cppunit/extensions/HelperMacros.h>

#include "Types.h"

using namespace Minotaur;

class LoggerUT : public CppUnit::TestFixture {

public:

  void setUp() { }    // need not implement
  void tearDown() { } // need not implement

  CPPUNIT_TEST_SUITE(LoggerUT);
  CPPUNIT_TEST(testMsgStream);
  CPPUNIT_TEST(testErrStream);
  CPPUNIT_TEST_SUITE_END();

  void testMsgStream();
  void testErrStream();

};

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
