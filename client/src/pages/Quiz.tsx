import { FC, useEffect, useState } from 'react';
import { useLocation } from 'react-router-dom';
import { Voting } from '../components/Quiz/Voting/Voting';
import { Lobby } from '../components/Quiz/Lobby/Lobby';
import { useAuth } from '../hooks/useAuth';
import { socket } from '../services/socket';
import { User } from '../types/User';
import { Match } from '../components/Quiz/Match/Match';
import { End } from '../components/Quiz/End';
import { Question } from '../types/Question';
import { updateUser } from '../services/requests';

export const Quiz: FC = () => {
  const [quizState, setQuizState] = useState<'waiting' | 'voting' | 'playing' | 'end'>('waiting');
  const [players, setPlayers] = useState<User[]>([]);
  const [winner, setWinner] = useState<User>();
  const [categories, setCategories] = useState<Record<number, string>>([]);
  const [category, setCategory] = useState<string>('');
  const [firstQuestion, setFirstQuestion] = useState<Question | null>(null);
  const { user } = useAuth();
  const location = useLocation();
  const { update } = useAuth();

  useEffect(() => {
    const roomPlayers = location.state.roomPlayers as User[];
    setPlayers(roomPlayers);

    // Listening to messages from the WebSocket server
    socket.onmessage = (event) => {
      const message = JSON.parse(event.data);
      console.log(message);
      
      switch (message.action) {
        case 'joinRoom':
          setPlayers(message.data.roomPlayers);
          break;

          case 'startVoting':
            console.log('start voting');
            
    
            // Transform the categories array into a Record<number, string>
            const transformedCategories: Record<number, string> = message.data.categories.reduce((acc: Record<number, string>, category: { id: string, name: string }) => {
              acc[parseInt(category.id)] = category.name;  // Ensure the key is a number (parseInt)
              return acc;
            }, {});
    
            // Set the transformed categories
            setCategories(transformedCategories);
            setQuizState('voting');
    
            break;

        case 'startMatch':
          setQuizState('playing');
          setCategory(message.data.category);
          setFirstQuestion(message.data.firstQuestion);
          break;

        case 'gameOver':
          setWinner(players.find((player) => player.id === message.data.winnerId));
          setQuizState('end');
          break;

        default:
          console.log('Unknown message type', message.action);
      }
    };

    return () => {
      socket.onmessage = null; // Clean up on unmount
    };
  }, [players, location.state.roomPlayers]);

    useEffect(() => {
    console.log('categories received:', categories);
  }, [categories]);


  useEffect(() => {
    // Handle game over and update the user stats
    if (quizState === 'end' && winner) {
      if (user?.id === winner.id) {
        update({
          id: user?.id as string,
          wins: (user?.wins as number) + 1,
        });
        updateUser(user?.id as string, {
          id: user?.id as string,
          wins: (user?.wins as number) + 1,
        });
      }

      update({
        id: user?.id as string,
        playedGames: (user?.playedGames as number) + 1,
      });

      updateUser(user?.id as string, {
        id: user?.id as string,
        playedGames: (user?.playedGames as number) + 1,
      });
    }
  }, [quizState, winner]);

  // Function to send messages to the WebSocket server
  const sendMessage = (action: string, data: any) => {
    socket.send(JSON.stringify({ action, data }));
  };

  return (
    <div className='absolute inset-0 flex items-center justify-center px-8'>
      {quizState === 'waiting' && <Lobby players={players} />}
      {quizState === 'voting' && <Voting categories={categories} />}
      {quizState === 'playing' && (
        <Match
          category={category}
          firstQuestion={firstQuestion as Question}
          players={players}
          setPlayers={setPlayers}
        />
      )}
      {quizState === 'end' && <End winner={winner as User} players={players} />}
    </div>
  );
};
