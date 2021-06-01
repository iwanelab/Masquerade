#include "IdleWatcher.h"

IdleWatcher::IdleWatcher(QObject *parent)
	: QObject(parent)
{
	m_timer.setInterval(5000);
	m_timer.start();

	connect(&m_timer, SIGNAL(timeout()), this, SIGNAL(sgIdle()));
}

IdleWatcher::~IdleWatcher()
{

}
