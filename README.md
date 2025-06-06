# PogPool

Simple postgres pool managemenets / metaprogramming library.

## Idea


I wanted to create a connection pool that is easy to maintain. I know there is PGPool which is what should be used instead of this, but
I am okay rolling out my own for now.

I wanted to create some simple script that create a C function depending on a SQL file it was given to it.

The idea is simple. Given a list of table, create a basic CRUD functionality. If it has FK, then create CRUD with the joins if possible. 


## TODO 

- Join table supports
- Clean up the API so that it can be use easily.
- Migration support? (probably not?)
