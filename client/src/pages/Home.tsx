import { Link, useNavigate } from 'react-router-dom';
import {
  MagnifyingGlassIcon,
  LockClosedIcon,
  UserIcon,
} from '@heroicons/react/24/outline';
import { useEffect, useState } from 'react';
import { PrivateMatchModal } from '../components/PrivateMatchModal';
import { useAuth } from '../hooks/useAuth';
import { socket } from '../services/socket';  // Import socket from services
import { User } from '../types/User';

export const Home = () => {
  const [isPrivateMatchModalOpen, setIsPrivateMatchModalOpen] =
    useState<boolean>(false);
  const [players, setPlayers] = useState<User[]>([]);
  const { user } = useAuth();
  const navigate = useNavigate();

  useEffect(() => {
    document.title = 'QuizMaster';
    if (!user) navigate('../auth');

    // Handle WebSocket messages
    socket.onmessage = (event) => {
      const message = JSON.parse(event.data);

      switch (message.type) {
        case 'joinRoom':
          setPlayers(message.data.roomPlayers);
          navigate(`../quiz/${message.data.roomId}`, { state: { roomPlayers: message.data.roomPlayers } });
          break;

        case 'startVoting':
          console.log('Voting started', message.data.categories);
          break;

        case 'startMatch':
          console.log('Match started', message.data);
          break;

        case 'gameOver':
          console.log('Game over', message.data.winnerId);
          break;

        default:
          console.log('Unknown message type', message.type);
      }
    };

    return () => {
      socket.onmessage = null;
    };
  }, [user, navigate]);

  const findMatch = () => {
    if (user) {
      // Send the user data as a stringified JSON object
      const message = JSON.stringify({
        action: 'findMatch',
        data: user,
      });

      socket.send(message);  // Use socket.send() to send the message to the server
    }
  };

  const btnStyle =
    'w-80 rounded-lg bg-portage-400 transition hover:scale-110 hover:bg-portage-300 active:opacity-80 lg:h-64 drop-shadow flex items-center justify-center';
  const svgStyle = 'absolute h-full w-full opacity-10';

  return (
    <>
      <div className='flex h-[calc(100vh-76px)] w-full items-center justify-center gap-8 text-4xl font-bold text-gray-800'>
        <div className='grid h-[calc(100vh-150px)] grid-cols-1 grid-rows-3 gap-8 lg:grid-cols-3 lg:grid-rows-1 lg:items-center'>
          <button onClick={findMatch} className={btnStyle}>
            FIND MATCH
            <MagnifyingGlassIcon className={svgStyle} />
          </button>
          
          <Link
            to={`../profile/${user?.id}`}
            className={`${btnStyle} flex items-center justify-center`}
          >
            YOUR PROFILE
            <UserIcon className={svgStyle} />
          </Link>
        </div>
      </div>

    </>
  );
};
