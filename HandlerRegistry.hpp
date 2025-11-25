#pragma once

#include <vector>

#include "IHandler.hpp"

// Singleton registry for handler prototypes.
// Handlers are registered once at startup and used to match incoming requests.
class HandlerRegistry {
 public:
  // Get the singleton instance
  static HandlerRegistry& instance();

  // Register a handler prototype. Registry takes ownership.
  void registerHandler(IHandler* prototype);

  // Find and create a handler for the given context.
  // Returns NULL if no handler matches.
  // Caller takes ownership of the returned handler.
  IHandler* createHandler(const HandlerContext& ctx) const;

  // Clear all registered handlers (for cleanup)
  void clear();

 private:
  HandlerRegistry();
  ~HandlerRegistry();
  HandlerRegistry(const HandlerRegistry&);
  HandlerRegistry& operator=(const HandlerRegistry&);

  std::vector<IHandler*> prototypes_;
};
