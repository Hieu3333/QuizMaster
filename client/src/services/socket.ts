const socket = new WebSocket('ws://localhost:5000/'); // Use your WebSocket server URL

// When the connection is open, send a message in JSON format
socket.onopen = () => {
  console.log('Connected to the WebSocket server');

  // Create a JSON object with the desired structure
  const message = {
    action: 'greet',  // Action type (can be modified as needed)
    content: 'Hello from frontend!'  // Additional content
  };

  // Send the message as a JSON string
  socket.send(JSON.stringify(message));
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
