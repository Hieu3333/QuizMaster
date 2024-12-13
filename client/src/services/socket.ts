const socket = new WebSocket('ws://localhost:5000/'); // Use your WebSocket server URL

// When the connection is open, send a message in JSON format
socket.onopen = () => {
  console.log('Connected to the WebSocket server');

};

// Handle incoming messages
socket.onmessage = (event) => {
  try {
    const receivedMessage = JSON.parse(event.data);  // Parse the incoming JSON message
    console.log('Received message:', receivedMessage);
  } catch (error) {
    console.error('Error parsing incoming message:', error);
  }
};

socket.onclose = () => {
  console.log('WebSocket connection closed');
};

socket.onerror = (error) => {
  console.error('WebSocket Error:', error);
};

export { socket };
