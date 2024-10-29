from typing import Annotated

from fastapi import Depends, FastAPI, Query
from sqlmodel import Field, Session, SQLModel, create_engine, select
from datetime import datetime
import datetime as dt


class PriceVolData(SQLModel, table=True):
    id: int | None = Field(default=None, primary_key=True)
    timestamp: datetime = Field(index=True)
    volume: int
    price: int


sqlite_file_name = "orderbook.db"
sqlite_url = f"sqlite:///{sqlite_file_name}"

connect_args = {"check_same_thread": False}
engine = create_engine(sqlite_url, connect_args=connect_args)


def create_db_and_tables():
    SQLModel.metadata.create_all(engine)


def get_session():
    with Session(engine) as session:
        yield session


SessionDep = Annotated[Session, Depends(get_session)]

app = FastAPI()


@app.on_event("startup")
def on_startup():
    create_db_and_tables()


@app.get("/price-vol-data/")
def read_heroes(
    session: SessionDep,
    offset: int = 0,
    startdate: datetime | None = None,
    limit: Annotated[int, Query(le=100)] = 100,
) -> list[PriceVolData]:
    query = select(PriceVolData)
    query = query.offset(offset).limit(limit)
    entries = session.exec(query).all()
    if startdate:
        return [entry for entry in entries if entry.timestamp > startdate]
    return entries


@app.post("/price-vol-data/")
def create_hero(data: PriceVolData, session: SessionDep) -> PriceVolData:
    data.timestamp = datetime.fromisoformat(data.timestamp)
    session.add(data)
    session.commit()
    session.refresh(data)
    return data
