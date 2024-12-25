import { FC, useEffect, useState } from 'react';
import { useAuth } from '../../../hooks/useAuth';
import { updateUser } from '../../../services/requests';
import { socket } from '../../../services/socket';
import { Question } from '../../../types/Question';
import { User } from '../../../types/User';
import { Answers } from './Answers';
import { Scoreboard } from './Scoreboard';
import { useNavigate } from 'react-router-dom';

interface MatchProps {
  category: string;
  firstQuestion: Question;
  players: User[];
  setPlayers: React.Dispatch<React.SetStateAction<User[]>>;
  setQuizState: React.Dispatch<React.SetStateAction<'waiting' | 'voting'  | 'playing' | 'end'>>;
  setWinner: React.Dispatch<React.SetStateAction<User | undefined>>;
}

export const Match: FC<MatchProps> = ({
  category,
  firstQuestion,
  players,
  setPlayers,
  setQuizState,
  setWinner,
}) => {
  const { user, update } = useAuth();
  const navigate = useNavigate();
  const [hasAnswered, setHasAnswered] = useState<boolean>(false);
  const [updatedUser, setUpdatedUser] = useState<User>(user as User);
  const [currentQuestion, setCurrentQuestion] = useState<Question>(firstQuestion);
  const [hints, setHints] = useState<string[]>([]);
  const [action, setAction] = useState('');

  useEffect(() => {
    document.title = `QuizMaster | ${category}`;

    socket.onmessage = (event) => {
      const message = JSON.parse(event.data);

      if (message.action === 'answerResult') {
        const { isCorrect, playerId, answer } = message.data;

        if (isCorrect && playerId === user?.id) {
          update({
            id: user?.id as string,
            totalScore: (updatedUser?.totalScore as number) + 1,
          });

          updateUser(user?.id as string, {
            id: user?.id as string,
            totalScore: (updatedUser?.totalScore as number) + 1,
          });

          setUpdatedUser((prevUser) => ({
            ...prevUser,
            totalScore: (prevUser?.totalScore as number) + 1,
          }));
        }

        setHints((prevHints) => {
          const { username } = players.find(
            (player) => player.id === playerId
          ) as User;
          const newHint =
            username +
            ' answered "' +
            answer +
            '" and was ' +
            (isCorrect ? 'correct' : 'wrong') +
            '.';
          if (!prevHints.includes(newHint)) prevHints?.push(newHint);
          return prevHints;
        });

        setPlayers((prevPlayers) =>
          prevPlayers.map((player) => {
            if (player.id === playerId) {
              return {
                ...player,
                score: isCorrect ? (player.score as number) + 1 : player.score,
              };
            }
            return player;
          })
        );
      }

      if (message.action === 'nextQuestion') {
        setCurrentQuestion(message.data);
        setHints([]);
        setHasAnswered(false);
      }

      if (message.action === 'playerQuit'){
        setPlayers((prevPlayers) => prevPlayers.filter(player => player.id !== message.data.playerId));
      }

      if (message.action === 'gameOver') {
        const winnerId = message.data.winnerId;
        const winner = players.find((player) => player.id === winnerId);
        setWinner(winner);
        setQuizState('end');
      }
    };

    return () => {
      socket.onmessage = null;
    };
  }, [updatedUser, user?.id, players, setWinner]);

  const handleAnswer = (answer: string) => {
    if (hasAnswered) return;

    socket.send(
      JSON.stringify({
        action: 'answer',
        data: { answer, playerId: user?.id as string },
      })
    );
    setHasAnswered(true);
  };

  const handleQuit = () => {
    // Notify the server about the quit action
    socket.send(
      JSON.stringify({
        action: 'quit',
        data: { playerId: user?.id as string },
      })
    );

    // Navigate back to the home page
    navigate('/');
  };

  return (
    <div className='mt-12 flex flex-col items-center lg:mt-0'>
      <Scoreboard players={players} />
      <div className='my-6 h-fit w-screen justify-center border-y border-black bg-white pb-4 text-center font-bold shadow-2xl shadow-black/25 sm:pb-8 md:my-12 md:pb-16 lg:pb-8'>
        <h3 className='pb-2 pt-4 text-lg text-gray-400 md:text-xl'>{category}</h3>
        <h2 className='px-4 text-2xl lg:text-4xl'>{currentQuestion.question}</h2>
      </div>
      <Answers
        hasAnswered={hasAnswered}
        answers={currentQuestion.choices}
        hints={hints}
        handleAnswer={handleAnswer}
      />
      {/* Quit Button at the Bottom Center */}
      <button
        onClick={handleQuit}
        className='mt-6 bg-red-500 text-white px-6 py-2 rounded shadow hover:bg-red-600'
      >
        Quit
      </button>
    </div>
  );
};
