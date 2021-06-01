#ifndef IDLEWATCHER_H
#define IDLEWATCHER_H

#include <QObject>
#include <QTimer>
#include <QEvent>

class IdleWatcher : public QObject
{
	Q_OBJECT

protected:
	bool eventFilter(QObject *obj, QEvent *ev)
	{
		if(ev->type() == QEvent::KeyPress || 
			ev->type() == QEvent::MouseMove)
		{
			// now reset your timer, for example
			m_timer.start();
		}

		return QObject::eventFilter(obj, ev);
	}

public:
	IdleWatcher(QObject *parent);
	~IdleWatcher();

private:
	QTimer m_timer;

signals:
	void sgIdle();
};

#endif // IDLEWATCHER_H
