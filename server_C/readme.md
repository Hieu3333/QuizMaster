/project-root
|-- src/
|   |-- auth.c            # Authentication logic (login, register, logout)
|   |-- user.c            # User-related logic (CRUD operations)
|   |-- db.c              # MongoDB connection and query handling
|   |-- router.c          # Handles HTTP request routing (maps to AuthService and UserService)
|   |-- server.c          # Main entry point for the HTTP server setup
|-- include/
|   |-- auth.h            # Header for auth.c
|   |-- user.h            # Header for user.c
|   |-- db.h              # Header for db.c
|   |-- router.h          # Header for router.c
|   |-- server.h          # Header for server.c
|-- Makefile              # Makefile to compile the project
|-- README.md             # Project documentation
