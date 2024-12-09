import { User } from './User';
import { Question } from './Question';

export interface ServerToClientMessages {
  joinRoom: { roomId: number; roomPlayers: User[] };
  createdRoom: { roomId: number };
  startVoting: { categories: Record<number, string> };
  startMatch: { category: string; firstQuestion: Question };
  answerResult: { isCorrect: boolean; playerId: string; answer: string };
  nextQuestion: { question: Question };
  gameOver: { winnerId: string };
}

export interface ClientToServerMessages {
  findMatch: { user: User };
  vote: { category: string; playerId: string };
  answer: { answer: string; playerId: string };
}
