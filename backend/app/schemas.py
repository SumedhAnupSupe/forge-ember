from typing import Literal
from pydantic import BaseModel, EmailStr, Field, ConfigDict

RecipientType = Literal["TO", "CC", "BCC"]


class UserOut(BaseModel):
    id: int
    name: str
    email: EmailStr
    created_at: int = 0


class AuthRequest(BaseModel):
    email: EmailStr
    password: str = Field(min_length=8, max_length=1024)


class RegisterRequest(AuthRequest):
    name: str = Field(min_length=1, max_length=63)


class AuthResponse(BaseModel):
    user: UserOut
    token: str


class RecipientInput(BaseModel):
    email: EmailStr
    type: RecipientType = "TO"


class SendMailRequest(BaseModel):
    recipients: list[RecipientInput] = Field(min_length=1, max_length=100)
    subject: str = Field(max_length=127)
    body: str = Field(max_length=511)


class RecipientOut(BaseModel):
    id: int
    name: str
    email: EmailStr
    type: RecipientType


class SenderOut(BaseModel):
    id: int
    name: str
    email: EmailStr
    model_config = ConfigDict(extra="ignore")


class MessageOut(BaseModel):
    id: int
    sender: SenderOut
    recipients: list[RecipientOut]
    subject: str
    body: str
    created_at: int
    is_read: bool = False
    is_starred: bool = False
    is_deleted: bool = False


class StateUpdate(BaseModel):
    value: bool
