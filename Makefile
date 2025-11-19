# Wrapper Makefile for CMake build system
# This maintains compatibility with the original Makefile interface

NAME := webserv
BUILD_DIR := build

# Default target
all: $(NAME)

# Build the project using CMake
$(NAME):
	@mkdir -p $(BUILD_DIR)
	@cd $(BUILD_DIR) && cmake .. $(if $(LOG_LEVEL),-DLOG_LEVEL=$(LOG_LEVEL),)
	@cd $(BUILD_DIR) && $(MAKE) --no-print-directory
	@cp $(BUILD_DIR)/$(NAME) .

# Clean build artifacts (object files, etc.)
clean:
	@if [ -d $(BUILD_DIR) ]; then cd $(BUILD_DIR) && $(MAKE) --no-print-directory clean; fi
	@rm -f $(NAME)

# Full clean (remove build directory)
fclean: clean
	@rm -rf $(BUILD_DIR)

# Rebuild from scratch
re: fclean all

# Help target to show available commands
help:
	@echo "Available targets:"
	@echo "  all     - Build the project (default)"
	@echo "  clean   - Remove build artifacts"
	@echo "  fclean  - Remove all generated files including build directory"
	@echo "  re      - Rebuild from scratch (fclean + all)"
	@echo "  help    - Show this help message"
	@echo ""
	@echo "Build options:"
	@echo "  LOG_LEVEL=N - Set log level (0=DEBUG, 1=INFO, 2=ERROR)"
	@echo "  Example: make LOG_LEVEL=0"

.PHONY: all clean fclean re help
