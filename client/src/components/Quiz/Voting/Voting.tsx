import { FC, useEffect, useState } from 'react';
import { Category } from './Category';
import { socket } from '../../../services/socket'; // Update to use WebSocket service
import { useAuth } from '../../../hooks/useAuth';

interface VotingProps {
  categories: Record<number, string>;
}

export const Voting: FC<VotingProps> = ({ categories }) => {
  const { user } = useAuth();
  const [hasVoted, setHasVoted] = useState<boolean>(false);

  useEffect(() => {
    document.title = 'QuizMaster | Voting';

    // Listen for vote result updates via WebSocket
    socket.onmessage = (event) => {
      const message = JSON.parse(event.data);
      if (message.action === 'voteResult') {
        const { playerId } = message.data;
        if (playerId === user?.id) {
          setHasVoted(true); // Mark that the user has voted after receiving confirmation
        }
      }
    };

    return () => {
      socket.onmessage = null; // Cleanup the WebSocket message handler when the component unmounts
    };
  }, [user?.id]);

  const handleVote = (key: string) => {
    if (hasVoted) return; // Prevent voting again if already voted

    // Send the vote via WebSocket
    socket.send(
      JSON.stringify({
        action: 'vote',
        data: { category: key, playerId: user?.id as string },
      })
    );
    setHasVoted(true); // Optimistically set the state to reflect the vote
  };

  return (
    <div className='flex flex-col items-center rounded-xl bg-white p-8'>
      <h3 className='mb-8 rounded-xl text-2xl font-bold text-black'>
        Choose a category!
      </h3>
      <ul className='grid grid-cols-1 grid-rows-6 justify-center gap-4 rounded-xl bg-slate-300 p-4 md:grid-cols-2 md:grid-rows-3'>
        {Object.entries(categories).map(([key, value], i) => (
          <Category
            handleVote={handleVote}
            key={i}
            categoryIndex={key}
            categoryText={value}
            hasVoted={hasVoted}
          />
        ))}
      </ul>
    </div>
  );
};
