from typing import Annotated
from starlette.responses import FileResponse
from fastapi import Depends, FastAPI, Query
from sqlmodel import Field, Session, SQLModel, create_engine, select, Column
from sqlalchemy import Integer
from datetime import datetime
from fastapi.middleware.cors import CORSMiddleware
from enum import Enum


class PriceVolData(SQLModel, table=True):
    id: int | None = Field(default=None, primary_key=True)
    timestamp: datetime = Field(index=True)
    volume: int
    price: int


class OrderData(SQLModel, table=True):
    id: int | None = Field(default=None, primary_key=True)
    timestamp: datetime = Field(index=True)
    ordertype: int
    side: int
    price: int
    quantity: int


sqlite_file_name = "../orderbook.db"
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

app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"],
)


@app.on_event("startup")
def on_startup():
    create_db_and_tables()


@app.get("/price-vol-data/")
def read_pricevoldata(
    session: SessionDep,
    offset: int = 0,
    startdate: datetime | None = None,
    limit: Annotated[int, Query(le=1000)] = 1000,
) -> list[PriceVolData]:
    query = select(PriceVolData)
    query = query.offset(offset).limit(
        limit).order_by(PriceVolData.timestamp.desc())
    entries = session.exec(query).all()
    if startdate:
        return [entry for entry in entries if entry.timestamp > startdate]
    return entries


@app.get("/orders/")
def read_orders(
    session: SessionDep,
    offset: int = 0,
    startdate: datetime | None = None,
    limit: Annotated[int, Query(le=1000)] = 1000,
) -> list[OrderData]:
    query = select(OrderData)
    query = query.offset(offset).limit(limit).order_by(OrderData.timestamp)
    entries = session.exec(query).all()
    if startdate:
        return [entry for entry in entries if entry.timestamp > startdate]
    return entries


@app.post("/price-vol-data/")
def create_pricevoldata(data: PriceVolData, session: SessionDep) -> PriceVolData:
    data.timestamp = datetime.fromisoformat(data.timestamp)
    session.add(data)
    session.commit()
    session.refresh(data)
    return data


@app.get("/")
def read_index():
    return FileResponse("index.html")
