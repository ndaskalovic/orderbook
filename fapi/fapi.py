from typing import Annotated
from starlette.responses import FileResponse
from fastapi import Depends, FastAPI, HTTPException, Query, Response
from sqlmodel import Field, Session, SQLModel, create_engine, select
from datetime import datetime
from fastapi.middleware.cors import CORSMiddleware
from cpu_info import get_cpu_info

CPU_INFO = get_cpu_info()


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


class OrderPressure(SQLModel, table=True):
    id: int | None = Field(default=None, primary_key=True)
    ratio: int
    overwrite: bool


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


@app.get("/orderbook-api/price-vol-data/")
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


@app.get("/orderbook-api/orders/")
def read_orders(
    session: SessionDep,
    offset: int = 0,
    startdate: datetime | None = None,
    limit: Annotated[int, Query(le=100)] = 100,
) -> list[OrderData]:
    query = select(OrderData)
    query = query.offset(offset).limit(
        limit).order_by(OrderData.timestamp.desc())
    entries = session.exec(query).all()
    if startdate:
        return [entry for entry in entries if entry.timestamp > startdate][::-1]
    return entries[::-1]


@app.post("/orderbook-api/price-vol-data/")
def create_pricevoldata(data: PriceVolData, session: SessionDep) -> PriceVolData:
    data.timestamp = datetime.fromisoformat(data.timestamp)
    session.add(data)
    session.commit()
    session.refresh(data)
    return data


@app.get("/")
def read_index():
    return FileResponse("index.html")


@app.get("/orderbook-api/platform-info/")
def read_cpu_info():
    return Response(CPU_INFO, 200)


@app.post("/orderbook-api/side-ratio/")
def create_pressure(data: OrderPressure, session: SessionDep) -> OrderPressure:
    order_pressure = session.exec(
        select(OrderPressure).where(OrderPressure.id == 1)).first()

    if not order_pressure:
        raise HTTPException(
            status_code=404, detail="OrderPressure with ID 1 not found")

    order_pressure.ratio = data.ratio
    session.add(order_pressure)
    session.commit()
    session.refresh(order_pressure)

    return order_pressure
