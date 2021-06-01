//
// HMACEngineTest.cpp
//
// $Id: //poco/1.3/Foundation/testsuite/src/HMACEngineTest.cpp#1 $
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


#include "HMACEngineTest.h"
#include "CppUnit/TestCaller.h"
#include "CppUnit/TestSuite.h"
#include "Poco/HMACEngine.h"
#include "Poco/MD5Engine.h"


using Poco::HMACEngine;
using Poco::MD5Engine;
using Poco::DigestEngine;


HMACEngineTest::HMACEngineTest(const std::string& name): CppUnit::TestCase(name)
{
}


HMACEngineTest::~HMACEngineTest()
{
}


void HMACEngineTest::testHMAC()
{
	// test vectors from RFC 2104
	
	std::string key(16, 0x0b);
	std::string data("Hi There");
	HMACEngine<MD5Engine> hmac1(key);
	hmac1.update(data);
	std::string digest = DigestEngine::digestToHex(hmac1.digest());
	assert (digest == "9294727a3638bb1c13f48ef8158bfc9d");
	
	key  = "Jefe";
	data = "what do ya want for nothing?";
	HMACEngine<MD5Engine> hmac2(key);
	hmac2.update(data);
	digest = DigestEngine::digestToHex(hmac2.digest());
	assert (digest == "750c783e6ab0b503eaa86e310a5db738");
	
	key  = std::string(16, 0xaa);
	data = std::string(50, 0xdd);
	HMACEngine<MD5Engine> hmac3(key);
	hmac3.update(data);
	digest = DigestEngine::digestToHex(hmac3.digest());
	assert (digest == "56be34521d144c88dbb8c733f0e8b3f6");
}


void HMACEngineTest::setUp()
{
}


void HMACEngineTest::tearDown()
{
}


CppUnit::Test* HMACEngineTest::suite()
{
	CppUnit::TestSuite* pSuite = new CppUnit::TestSuite("HMACEngineTest");

	CppUnit_addTest(pSuite, HMACEngineTest, testHMAC);

	return pSuite;
}
