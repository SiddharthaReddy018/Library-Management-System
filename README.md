# Library Management System

A concurrent library management system implemented in C using socket programming and POSIX threads.

## Features

### Admin Features
- Add/Delete/Search books
- Add/Delete/Search members
- Add new admin accounts
- View all books
- View borrowed books
- Concurrent user session management

### User Features
- Search books
- Borrow books
- Return books
- Session-based authentication

## Technical Implementation

### Concurrency Control
- Multi-threaded server using POSIX threads
- Mutex locks for critical section handling
- Thread-safe file operations
- Atomic operations for book transactions

### Network Programming
- TCP/IP socket programming
- Client-server architecture
- Multiple concurrent client connections
- Full-duplex communication

### Security
- Role-based access control (Admin/User)
- Password-based authentication
- Single session per user enforcement
- Protected file operations

## System Architecture

### Server Components
- Main server thread (accepts connections)
- Client handler threads (one per connection)
- Mutex-protected shared resources
- File system interface

### Client Components
- Socket connection management
- User interface
- Request/response handling
- Session management

## Data Storage
The system uses three main files:
- `books.txt`: Stores book information (ISBN, Title, Author, Availability)
- `users.txt`: Stores user credentials and roles
- `borrowed_books.txt`: Tracks borrowed books and borrowers

## Building and Running

### Prerequisites
- GCC compiler
- POSIX-compliant operating system
- pthread library

### Compilation
```bash
# Compile server
gcc server.c -o server -pthread

# Compile client
gcc client.c -o client
```

### Running the Application
```bash
# Start server
./server

# Start client
./client
```

## Usage

### Admin Login
1. Launch the client
2. Login with admin credentials
3. Access admin menu for management operations

### User Login
1. Launch the client
2. Login with user credentials
3. Access user menu for book operations

## Technical Details

### Thread Synchronization
- Mutex locks protect shared resources
- Critical sections are handled atomically
- File operations are thread-safe
- Concurrent access is managed efficiently

### Error Handling
- Socket errors
- File operation errors
- Authentication failures
- Invalid input handling

### Memory Management
- Dynamic memory allocation for connections
- Proper resource cleanup
- Thread detachment handling
- Buffer management

## Security Considerations
- Password protection
- Role-based access
- Session management
- Protected file operations
- Input validation

## Project Structure
```
.
├── server.c          # Server implementation
├── client.c          # Client implementation
├── books.txt         # Book database
├── users.txt         # User credentials
└── borrowed_books.txt # Borrowed books tracking
```

## Future Enhancements
- Encrypted communication
- Password hashing
- Database integration
- Backup system
- Activity logging
- Book reservation system

## License
This project is licensed under the MIT License - see the LICENSE file for details.
