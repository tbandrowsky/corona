# Copilot Instructions

## Project Guidelines
- User prefers icon glyph string literals using Unicode escape format like u"\E7414" rather than long UTF-8 byte sequences.
- User is working on Natvis visualizers for Corona JSON types and may correct ambiguous references (e.g., meant natvis rather than visx).

## Application Behavior
- Startup issues with page/config updates can occur because `apply_schema` still needs to call `create_application` to refresh application/page state without full database recreation.

