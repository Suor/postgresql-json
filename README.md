# JSON extraction for PostgreSQL

There is limited support for JSON in PostgreSQL 9.2, you can create JSON values,
but you can't query them. This little library aimed at solving that shortcoming.

## Installation

```bash
git clone git://github.com/Suor/postgresql-json.git
cd postgresql-json
./build -d DATABASE -u USER [-a ADMIN_USER] [-t FIELD_TYPE]
```

## Usage

```sql
select * from plays where json_string(json_doc, 'author') = 'Shakespeare'
                      and json_int(json_doc, 'year') = 1611
                      and json_bool(json_doc, 'published');
```

## CAVEATS

1. Only string, int and boolean properties are supported for now.
2. Extracting nesting properties is not supported.
