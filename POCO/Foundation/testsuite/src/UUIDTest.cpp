//
// UUIDTest.cpp
//
// $Id: //poco/1.3/Foundation/testsuite/src/UUIDTest.cpp#1 $
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


#include "UUIDTest.h"
#include "CppUnit/TestCaller.h"
#include "CppUnit/TestSuite.h"
#include "Poco/UUID.h"


using Poco::UUID;


UUIDTest::UUIDTest(const std::string& name): CppUnit::TestCase(name)
{
}


UUIDTest::~UUIDTest()
{
}


void UUIDTest::testParse()
{
	UUID uuid("6ba7b810-9dad-11d1-80b4-00c04fd430c8");
	assert (uuid.toString() == "6ba7b810-9dad-11d1-80b4-00c04fd430c8");
	
	uuid.parse("6BA7B810-9DAD-11D1-80B4-00C04FD430C8");
	assert (uuid.toString() == "6ba7b810-9dad-11d1-80b4-00c04fd430c8");	
}


void UUIDTest::testBuffer()
{
	UUID uuid("6ba7b810-9dad-11d1-80b4-00c04fd430c8");
	char buffer[16];
	uuid.copyTo(buffer);
	UUID uuid2;
	uuid2.copyFrom(buffer);
	assert (uuid2.toString() == "6ba7b810-9dad-11d1-80b4-00c04fd430c8");	
}


void UUIDTest::testCompare()
{
	UUID nil;
	assert (nil.isNil());
	assert (UUID::nil().isNil());
	
	UUID uuid1 = nil;
	UUID uuid2;
	assert (uuid1.isNil());
	assert (uuid1 == nil);
	assert (!(uuid1 != nil));
	assert (uuid1 >= nil);
	assert (uuid1 <= nil);
	assert (!(uuid1 > nil));
	assert (!(uuid1 < nil));
	assert (uuid1.toString() == "00000000-0000-0000-0000-000000000000");
	
	uuid1 = UUID::dns();
	assert (!uuid1.isNil());
	assert (uuid1 != nil);
	assert (!(uuid1 == nil));
	assert (uuid1 >= nil);
	assert (!(uuid1 <= nil));
	assert (uuid1 > nil);
	assert (!(uuid1 < nil));
	assert (uuid1.toString() == "6ba7b810-9dad-11d1-80b4-00c04fd430c8");

	assert (nil != uuid1);
	assert (!(nil == uuid1));
	assert (!(nil >= uuid1));
	assert (nil <= uuid1);
	assert (!(nil > uuid1));
	assert (nil < uuid1);
	
	uuid2 = uuid1;
	assert (uuid2 == uuid1);
	assert (!(uuid2 != uuid1));
	assert (uuid2 >= uuid1);
	assert (uuid2 <= uuid1);
	assert (!(uuid2 > uuid1));
	assert (!(uuid2 < uuid1));
}


void UUIDTest::testVersion()
{
	UUID uuid("db4fa7e9-9e62-4597-99e0-b1ec0b59800e");
	UUID::Version v = uuid.version();
	assert (v == UUID::UUID_RANDOM);
	
	uuid.parse("6ba7b810-9dad-11d1-80b4-00c04fd430c8");
	v = uuid.version();
	assert (v == UUID::UUID_TIME_BASED);

	uuid.parse("d2ee4220-3625-11d9-9669-0800200c9a66");
	v = uuid.version();
	assert (v == UUID::UUID_TIME_BASED);

	uuid.parse("360d3652-4411-4786-bbe6-b9675b548559");
	v = uuid.version();
	assert (v == UUID::UUID_RANDOM);
}


void UUIDTest::testVariant()
{
	UUID uuid("db4fa7e9-9e62-4597-99e0-b1ec0b59800e");
	int v = uuid.variant();
	assert (v == 2);
	
	uuid.parse("6ba7b810-9dad-11d1-80b4-00c04fd430c8");
	v = uuid.variant();
	assert (v == 2);

	uuid.parse("d2ee4220-3625-11d9-9669-0800200c9a66");
	v = uuid.variant();
	assert (v == 2);

	uuid.parse("360d3652-4411-4786-bbe6-b9675b548559");
	v = uuid.variant();
	assert (v == 2);
}


void UUIDTest::testSwap()
{
	UUID uuid1("db4fa7e9-9e62-4597-99e0-b1ec0b59800e");
	UUID uuid2("d2ee4220-3625-11d9-9669-0800200c9a66");
	uuid1.swap(uuid2);
	assert (uuid1.toString() == "d2ee4220-3625-11d9-9669-0800200c9a66");
	assert (uuid2.toString() == "db4fa7e9-9e62-4597-99e0-b1ec0b59800e");
}


void UUIDTest::setUp()
{
}


void UUIDTest::tearDown()
{
}


CppUnit::Test* UUIDTest::suite()
{
	CppUnit::TestSuite* pSuite = new CppUnit::TestSuite("UUIDTest");

	CppUnit_addTest(pSuite, UUIDTest, testParse);
	CppUnit_addTest(pSuite, UUIDTest, testBuffer);
	CppUnit_addTest(pSuite, UUIDTest, testCompare);
	CppUnit_addTest(pSuite, UUIDTest, testVersion);
	CppUnit_addTest(pSuite, UUIDTest, testVariant);
	CppUnit_addTest(pSuite, UUIDTest, testSwap);

	return pSuite;
}
