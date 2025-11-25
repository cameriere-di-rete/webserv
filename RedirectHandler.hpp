#pragma once

#include "IHandler.hpp"
#include "Location.hpp"

class RedirectHandler : public IHandler {
 public:
	explicit RedirectHandler(const Location& location);
	virtual ~RedirectHandler();

	virtual HandlerResult start(Connection& conn);
	virtual HandlerResult resume(Connection& conn);

 private:
	Location location_;
};
