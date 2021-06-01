//
// LinearHashTableTest.cpp
//
// $Id: //poco/1.3/Foundation/testsuite/src/LinearHashTableTest.cpp#3 $
//
// Copyright (c) 2006, Applied Informatics Software Engineering GmbH.
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


#include "LinearHashTableTest.h"
#include "CppUnit/TestCaller.h"
#include "CppUnit/TestSuite.h"
#include "Poco/LinearHashTable.h"
#include "Poco/HashTable.h"
#include "Poco/Stopwatch.h"
#include "Poco/NumberFormatter.h"
#include <set>
#include <iostream>


using Poco::LinearHashTable;
using Poco::Hash;
using Poco::HashTable;
using Poco::Stopwatch;
using Poco::NumberFormatter;


LinearHashTableTest::LinearHashTableTest(const std::string& name): CppUnit::TestCase(name)
{
}


LinearHashTableTest::~LinearHashTableTest()
{
}


void LinearHashTableTest::testInsert()
{
	const int N = 1000;

	LinearHashTable<int, Hash<int> > ht;
	
	assert (ht.empty());
	
	for (int i = 0; i < N; ++i)
	{
		std::pair<LinearHashTable<int, Hash<int> >::Iterator, bool> res = ht.insert(i);
		assert (*res.first == i);
		assert (res.second);
		LinearHashTable<int, Hash<int> >::Iterator it = ht.find(i);
		assert (it != ht.end());
		assert (*it == i);
		assert (ht.size() == i + 1);
	}		
	
	assert (!ht.empty());
	
	for (int i = 0; i < N; ++i)
	{
		LinearHashTable<int, Hash<int> >::Iterator it = ht.find(i);
		assert (it != ht.end());
		assert (*it == i);
	}
	
	for (int i = 0; i < N; ++i)
	{
		std::pair<LinearHashTable<int, Hash<int> >::Iterator, bool> res = ht.insert(i);
		assert (*res.first == i);
		assert (!res.second);
	}		
}


void LinearHashTableTest::testErase()
{
	const int N = 1000;

	LinearHashTable<int, Hash<int> > ht;

	for (int i = 0; i < N; ++i)
	{
		ht.insert(i);
	}
	assert (ht.size() == N);
	
	for (int i = 0; i < N; i += 2)
	{
		ht.erase(i);
		LinearHashTable<int, Hash<int> >::Iterator it = ht.find(i);
		assert (it == ht.end());
	}
	assert (ht.size() == N/2);
	
	for (int i = 0; i < N; i += 2)
	{
		LinearHashTable<int, Hash<int> >::Iterator it = ht.find(i);
		assert (it == ht.end());
	}
	
	for (int i = 1; i < N; i += 2)
	{
		LinearHashTable<int, Hash<int> >::Iterator it = ht.find(i);
		assert (it != ht.end());
		assert (*it == i);
	}

	for (int i = 0; i < N; i += 2)
	{
		ht.insert(i);
	}
	
	for (int i = 0; i < N; ++i)
	{
		LinearHashTable<int, Hash<int> >::Iterator it = ht.find(i);
		assert (it != ht.end());
		assert (*it == i);
	}
}


void LinearHashTableTest::testIterator()
{
	const int N = 1000;

	LinearHashTable<int, Hash<int> > ht;

	for (int i = 0; i < N; ++i)
	{
		ht.insert(i);
	}
	
	std::set<int> values;
	LinearHashTable<int, Hash<int> >::Iterator it = ht.begin();
	while (it != ht.end())
	{
		assert (values.find(*it) == values.end());
		values.insert(*it);
		++it;
	}
	
	assert (values.size() == N);
}


void LinearHashTableTest::testConstIterator()
{
	const int N = 1000;

	LinearHashTable<int, Hash<int> > ht;

	for (int i = 0; i < N; ++i)
	{
		ht.insert(i);
	}

	std::set<int> values;
	LinearHashTable<int, Hash<int> >::ConstIterator it = ht.begin();
	while (it != ht.end())
	{
		assert (values.find(*it) == values.end());
		values.insert(*it);
		++it;
	}
	
	assert (values.size() == N);
	
	values.clear();
	const LinearHashTable<int, Hash<int> > cht(ht);

	LinearHashTable<int, Hash<int> >::ConstIterator cit = cht.begin();
	while (cit != cht.end())
	{
		assert (values.find(*cit) == values.end());
		values.insert(*cit);
		++cit;
	}
	
	assert (values.size() == N);	
}


void LinearHashTableTest::testPerformanceInt()
{
	const int N = 5000000;
	Stopwatch sw;

	{
		LinearHashTable<int, Hash<int> > lht(N);
		sw.start();
		for (int i = 0; i < N; ++i)
		{
			lht.insert(i);
		}
		sw.stop();
		std::cout << "Insert LHT: " << sw.elapsedSeconds() << std::endl;
		sw.reset();
		
		sw.start();
		for (int i = 0; i < N; ++i)
		{
			lht.find(i);
		}
		sw.stop();
		std::cout << "Find LHT: " << sw.elapsedSeconds() << std::endl;
		sw.reset();
	}

	{
		HashTable<int, int> ht;
		
		sw.start();
		for (int i = 0; i < N; ++i)
		{
			ht.insert(i, i);
		}
		sw.stop();
		std::cout << "Insert HT: " << sw.elapsedSeconds() << std::endl;
		sw.reset();
		
		sw.start();
		for (int i = 0; i < N; ++i)
		{
			ht.exists(i);
		}
		sw.stop();
		std::cout << "Find HT: " << sw.elapsedSeconds() << std::endl;
	}
	
	{
		std::set<int> s;
		sw.start();
		for (int i = 0; i < N; ++i)
		{
			s.insert(i);
		}
		sw.stop();
		std::cout << "Insert set: " << sw.elapsedSeconds() << std::endl;
		sw.reset();
		
		sw.start();
		for (int i = 0; i < N; ++i)
		{
			s.find(i);
		}
		sw.stop();
		std::cout << "Find set: " << sw.elapsedSeconds() << std::endl;
		sw.reset();
	}
	
}


void LinearHashTableTest::testPerformanceStr()
{
	const int N = 5000000;
	Stopwatch sw;
	
	std::vector<std::string> values;
	for (int i = 0; i < N; ++i)
	{
		values.push_back(NumberFormatter::format0(i, 8));
	}

	{
		LinearHashTable<std::string, Hash<std::string> > lht(N);
		sw.start();
		for (int i = 0; i < N; ++i)
		{
			lht.insert(values[i]);
		}
		sw.stop();
		std::cout << "Insert LHT: " << sw.elapsedSeconds() << std::endl;
		sw.reset();
		
		sw.start();
		for (int i = 0; i < N; ++i)
		{
			lht.find(values[i]);
		}
		sw.stop();
		std::cout << "Find LHT: " << sw.elapsedSeconds() << std::endl;
		sw.reset();
	}

	{
		HashTable<std::string, int> ht;
		
		sw.start();
		for (int i = 0; i < N; ++i)
		{
			ht.insert(values[i], i);
		}
		sw.stop();
		std::cout << "Insert HT: " << sw.elapsedSeconds() << std::endl;
		sw.reset();
		
		sw.start();
		for (int i = 0; i < N; ++i)
		{
			ht.exists(values[i]);
		}
		sw.stop();
		std::cout << "Find HT: " << sw.elapsedSeconds() << std::endl;
	}
	
	{
		std::set<std::string> s;
		sw.start();
		for (int i = 0; i < N; ++i)
		{
			s.insert(values[i]);
		}
		sw.stop();
		std::cout << "Insert set: " << sw.elapsedSeconds() << std::endl;
		sw.reset();
		
		sw.start();
		for (int i = 0; i < N; ++i)
		{
			s.find(values[i]);
		}
		sw.stop();
		std::cout << "Find set: " << sw.elapsedSeconds() << std::endl;
		sw.reset();
	}
}


void LinearHashTableTest::setUp()
{
}


void LinearHashTableTest::tearDown()
{
}


CppUnit::Test* LinearHashTableTest::suite()
{
	CppUnit::TestSuite* pSuite = new CppUnit::TestSuite("LinearHashTableTest");

	CppUnit_addTest(pSuite, LinearHashTableTest, testInsert);
	CppUnit_addTest(pSuite, LinearHashTableTest, testErase);
	CppUnit_addTest(pSuite, LinearHashTableTest, testIterator);
	CppUnit_addTest(pSuite, LinearHashTableTest, testConstIterator);
	//CppUnit_addTest(pSuite, LinearHashTableTest, testPerformanceInt);
	//CppUnit_addTest(pSuite, LinearHashTableTest, testPerformanceStr);

	return pSuite;
}
