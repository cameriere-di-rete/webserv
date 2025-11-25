#include "HandlerRegistry.hpp"

#include "Logger.hpp"

HandlerRegistry::HandlerRegistry() {}

HandlerRegistry::~HandlerRegistry() {
  clear();
}

HandlerRegistry& HandlerRegistry::instance() {
  static HandlerRegistry registry;
  return registry;
}

void HandlerRegistry::registerHandler(IHandler* prototype) {
  if (prototype) {
    prototypes_.push_back(prototype);
    LOG(DEBUG) << "HandlerRegistry: registered handler prototype";
  }
}

IHandler* HandlerRegistry::createHandler(const HandlerContext& ctx) const {
  for (std::vector<IHandler*>::const_iterator it = prototypes_.begin();
       it != prototypes_.end(); ++it) {
    if ((*it)->canHandle(ctx)) {
      LOG(DEBUG) << "HandlerRegistry: found matching handler for request";
      return (*it)->create(ctx);
    }
  }
  LOG(DEBUG) << "HandlerRegistry: no handler found for request";
  return NULL;
}

void HandlerRegistry::clear() {
  for (std::vector<IHandler*>::iterator it = prototypes_.begin();
       it != prototypes_.end(); ++it) {
    delete *it;
  }
  prototypes_.clear();
}
