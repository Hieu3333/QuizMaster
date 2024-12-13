const socket = new WebSocket('ws://localhost:5000/'); // Use your WebSocket server URL

socket.onopen = () => {
  console.log('Connected to the WebSocket server');
  socket.send('Hello from frontend!');
};

socket.onclose = () => {
  console.log('WebSocket connection closed');
};

socket.onerror = (error) => {
  console.error('WebSocket Error:', error);
};



export { socket };
