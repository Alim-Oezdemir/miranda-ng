#pragma once

#include "Observer.h"

#include <list>
class Subject : private MNonCopyable
{
public:
    Subject();
    virtual ~Subject();

    //--------------------------------------------------------------------------
    // Description : Notify all observers of a state change
    //--------------------------------------------------------------------------
    void notify();

    //--------------------------------------------------------------------------
    // Description : Attach an observer to the subject
    // Parameters  : observer - the observer
    //--------------------------------------------------------------------------
    void attach(Observer &observer);

    //--------------------------------------------------------------------------
    // Description : Detach an observer from the subject
    // Parameters  : observer - the observer
    //--------------------------------------------------------------------------
    void detach(const Observer &observer);

  private:
    std::list<Observer *> m_observer_list;
};

