CREATE TABLE Persons (
    personid UUID PRIMARY KEY DEFAULT uuid_generate_v4(),
    
    lastname VARCHAR(255),
    firstname VARCHAR(255),
    address VARCHAR(255),
    city VARCHAR(255),
);
