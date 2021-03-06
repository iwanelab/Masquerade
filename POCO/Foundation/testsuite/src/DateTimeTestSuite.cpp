//
// DateTimeTestSuite.cpp
//
// $Id: //poco/1.3/Foundation/testsuite/src/DateTimeTestSuite.cpp#1 $
//
// Copyright (c) 2004-2006, Applied Informatics Software Engineering GmbH.
// and Contributors.
//
// Permission is hereby granted, free of charge, to any person or organization
// obtaining a copy of the software and accompanying documentation covered by
// this license (the "Software") to use, reproduce, display, distribute,
// execute, and transmit the Software, and to prepare derivative works of the
// Software, and to permit third-parties to whom the Software is furnished to
// do so, all subject to the following:
// 
// The copyright notices in the Software and this entire statement, including
// the above license grant, this restriction and the following disclaimer,
// must be included in all copies of the Software, in whole or in part, and
// all derivative works of the Software, unless such copies or derivative
// works are solely in the form of machine-executable object code generated by
// a source language processor.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
// SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
// FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
// ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
//


#include "DateTimeTestSuite.h"
#include "TimestampTest.h"
#include "TimespanTest.h"
#include "TimezoneTest.h"
#include "DateTimeTest.h"
#include "LocalDateTimeTest.h"
#include "DateTimeFormatterTest.h"
#include "DateTimeParserTest.h"


CppUnit::Test* DateTimeTestSuite::suite()
{
	CppUnit::TestSuite* pSuite = new CppUnit::TestSuite("DateTimeTestSuite");

	pSuite->addTest(TimestampTest::suite());
	pSuite->addTest(TimespanTest::suite());
	pSuite->addTest(TimezoneTest::suite());
	pSuite->addTest(DateTimeTest::suite());
	pSuite->addTest(LocalDateTimeTest::suite());
	pSuite->addTest(DateTimeFormatterTest::suite());
	pSuite->addTest(DateTimeParserTest::suite());

	return pSuite;
}
