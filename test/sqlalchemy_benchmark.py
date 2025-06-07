import time
from sqlalchemy import create_engine, text

N_QUERIES = 1000
CONNINFO = "postgresql+psycopg2://pog_pool:pog_pool@localhost:4269/pog_pool"

def run_query(conn):
    conn.execute(text("SELECT * FROM ExampleTable"))

def test_with_pool(engine):
    start = time.time()
    with engine.connect() as conn:
        for _ in range(N_QUERIES):
            run_query(conn)
    end = time.time()
    #print(f"SQLAlchemy pooled: {end - start:.4f} seconds for {N_QUERIES} queries")

def test_without_pool():
    engine = create_engine(CONNINFO, poolclass=None)
    start = time.time()
    for _ in range(N_QUERIES):
        with engine.connect() as conn:
            run_query(conn)
    end = time.time()
    print(f"SQLAlchemy no pool: {end - start:.4f} seconds for {N_QUERIES} queries")

if __name__ == "__main__":
    engine = create_engine(CONNINFO, pool_size=10, max_overflow=0)
    start = time.time()
    times = 10
    for _ in range(times):
        test_with_pool(engine)
    end = time.time()
    print(f"Average time is: {((end - start)/(times * N_QUERIES))*1000:.4f} ms.")

    # Added in case we are curious
    # test_without_pool()

