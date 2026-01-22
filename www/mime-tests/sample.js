// JavaScript Test File
// Expected Content-Type: application/javascript or text/javascript

function greet(name) {
	console.log("Hello, " + name + "!");
	return "Hello, " + name + "!";
}

const message = greet("webserv");

// Sample object
const testObject = {
	name: "MIME Type Test",
	type: "JavaScript",
	version: 1.0
};

console.log(JSON.stringify(testObject, null, 2));
