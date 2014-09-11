# Basics

## Compile Objects

A **compiled object** is any object that requires an entry in the ``sysprocedures`` table.
The original source is stored in the ``syscomments`` table. 

``sp_helptext`` can be used to show the source.

## Identifiers

- Local variables start with ``@``.
- Global varaiables start with ``@@``
- Temporary table names must be preceded by ``tempdb`` if created inside ``tempdb``, or ``#`` if outside

``@@error`` is the global error var.

To check if an identifier is valid:


	select valid_name("MyIdentifier", 255)

where 255 is the max length. If valid, non-zero is returned.

The fully qualified database object name is:

	[[database.]owner.]object_name

For instance:

	pubs2.sharon.authors.city
	pubs2..authors.city

refers to the column city of table authors owned by sharon in database pubs2.

## Comment

	/* this is a comment */
	-- this is also a comment

# Queries

To switch to a different database:

	use mydb2

Renaming columnes in query:

	select "User Id" = id from users
	select id "User Id" from users
	select "Sum" = sum(amount) from sales

To display only the first 25 bytes of very long columnes, modify ``@@textsize``:

	set textsize 25
	set textsize 0 -- restore system default

Select unique values:

	select distinct id from users

The where clause:

	where name is null
	
	where sales between 100 and 1000  -- or 'not between'
	where state in ("CA", "IN", "MD")
	where id not in (select id from users)
	
	where phone not like "415%"   -- % matches any string of 0 or more chars
	where phone like "41_"        -- - matches a single char
	where phone like "[4-9]23"    -- range
	where phone like "[^4-9]23"   -- non-inclusion

To search for spec char themselves, include them in range:

	where phone like "41[_]"      -- use range to match special chars, more examples []], [[]

Or using the escape clause

	where phone like "415@%" escape "@"   -- search for 415%
	where phone like "%80@%%" escape "@"  -- search for string with 80%

Matching pattern can come directly from another table:

	select ... from tbl1, tbl2 where tbl1.name like tbl2.pat


Checking null

	where name is null
	where name is not null
	where name = null
	where name != null

Null column values do not join with other null column values.

Comparing null column values to other column values in ``where``  always returns **UNKNOWN**.

Note that the negative of ``UNKNONW`` is still ``UNKNOWN``.

To substitue a value for nulls:

	select isnull(name, "unknown") from titles

For type ``name char(30) null``, this is fine:

	where name is null
	where name = null

But for ``name text null``, have to use this

	where datalength(name) = 0

This evaluates to null:

	1 + null

This ignores null

	select "abc" + null + "def"
	------
	abcdef

Aggregate functions:

	sum([all | distinct] expression)
	avg([all | distinct] expression)
	count([all | distinct] expression) -- # of non-null values
	count(*)                           -- all, include null

Aggregate functions can not be used in where clause.

How having, group by and where interact

- where clause applied first
- group by applied next
- aggregate function applied on the result
- having applied last
