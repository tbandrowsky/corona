# coronaserver Overview

`coronaserver` is a C++20-based server application built on the CORONA framework. It provides a high-performance, low-code database engine for Windows, supporting dynamic schemas, user/team management, and secure data operations.

## Key Features

- **Schema-Driven Database Engine**: Define and manage classes (tables), fields, and indexes using JSON schemas.
- **User and Team Management**: Handles user accounts, authentication, password policies, and team-based permissions.
- **Granular Permissions**: Supports "get", "put", "delete", "alter" permissions at class and team levels.
- **Data Operations**: APIs for creating, editing, querying, copying, and deleting records.
- **Relational & NoSQL Storage**: Supports SQL Server and custom xtables for storage and indexing.
- **Security**: Uses cryptography for token generation and authentication.
- **Email Integration**: Onboarding and recovery via SendGrid.
- **Extensible Query Engine**: JSON-based query language for complex data joins.
- **OpenAPI Schema Generation**: Generates OpenAPI-compatible schemas for API documentation.

---

## Server Instructions

1. **Configure the Server**
   - Edit `config.json` to set up server credentials, email templates, and connection strings.
   - Example fields:
     - `Server`: Default user, password, email, team, API info, email templates.
     - `SendGrid`: API key, sender info, email titles.
     - `Connections`: Database and external service connection strings.

2. **Apply Schema**
   - Place your schema file (e.g., `revolution_schema.json`) in the deployment directory.
   - The server loads and applies this schema to define classes, fields, and relationships.

3. **Start the Server**
   - Build and run the `coronaserver` project in Visual Studio 2022.
   - The server initializes the database, applies configuration and schema, and starts listening for requests.

4. **API Usage**
   - Use the provided endpoints for user management, data operations, and queries.
   - Authentication is handled via encrypted tokens.

---

## revolution_schema.json Explained

- This file defines the database schema in JSON format.
- It contains arrays of class definitions, user objects, and datasets.
- Each class specifies its name, description, fields, indexes, and relationships.
- When deployed, the server reads this file and creates all necessary tables, fields, and indexes.

---

## config.json Explained

- This file configures server settings and external integrations.
- Key sections:
  - `Server`: Default credentials, team, API info, email templates.
  - `SendGrid`: Email sender info and API key for onboarding/recovery emails.
  - `Connections`: Connection strings for databases and services.
- Example:
  