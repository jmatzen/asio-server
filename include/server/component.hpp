#ifndef INCLUDED_component_h__
#define INCLUDED_component_h__

#include <boost/noncopyable.hpp>

namespace jm {
class Component : boost::noncopyable {
  public:
    virtual ~Component() {};

    static void preConstruct() {};

    virtual void postConstruct() {};
};

} // namespace jm

#endif // INCLUDED_component_h__