from typing import Annotated, Literal
from pydantic import BaseModel, EmailStr, Field, StringConstraints

ShortName = Annotated[str, StringConstraints(strip_whitespace=True, min_length=1, max_length=63)]
Password = Annotated[str, StringConstraints(min_length=8, max_length=1024)]
Subject = Annotated[str, StringConstraints(max_length=127)]
Body = Annotated[str, StringConstraints(max_length=511)]

class RegisterRequest(BaseModel):
    name: ShortName
    email: EmailStr
    password: Password

class LoginRequest(BaseModel):
    email: EmailStr
    password: str = Field(min_length=1, max_length=1024)

class UserOut(BaseModel):
    id: int
    name: str
    email: str
    created_at: int = 0

class LoginOut(BaseModel):
    token: str
    user: UserOut

class RecipientInput(BaseModel):
    user_id: int = Field(gt=0, le=4294967295)
    type: Literal["TO", "CC", "BCC"] = "TO"

class SendMessageRequest(BaseModel):
    subject: Subject
    body: Body
    recipients: list[RecipientInput] = Field(min_length=1, max_length=100)

class MessageCreated(BaseModel):
    id: int

class MessageOut(BaseModel):
    id: int
    sender_id: int
    subject: str
    body: str
    created_at: int
