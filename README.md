# Qt Learning Management System (QLMS)

A secure client-server application for course management built with Qt 6.9 and PostgreSQL.

## Features

- **Secure Communication**: SSL/TLS encrypted client-server communication
- **Role-Based Access Control**: Three user roles (Admin, Instructor, Student)
- **Multi-threaded Server**: Handles multiple concurrent client connections
- **PostgreSQL Database**: Robust data persistence

## User Roles

### Admin
- User management (create, delete, view all users)
- Full system access

### Instructor
- Create and manage quizzes
- Add questions (multiple choice, checkbox, open answer)
- Grade student quiz attempts

### Student
- View course materials (text lessons and quizzes)
- Take quizzes with enforced attempt limits
- Submit quiz answers for grading

## Prerequisites

- Qt 6.9 or later
- PostgreSQL 17 or later
- CMake 3.19 or later
- OpenSSL development libraries
- C++17 compatible compiler
