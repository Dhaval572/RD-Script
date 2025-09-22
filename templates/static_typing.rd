// Static typing example in Rubber Duck
// Variables must be explicitly declared with Auto keyword

Auto name = "Rubber Duck";        // String type
Auto age = 5;                     // Number type
Auto isScripting = true;          // Boolean type
Auto version = 1.0;               // Float type

Display("Language: " + name);
Display("Age: " + age);
Display("Is scripting: " + isScripting);
Display("Version: " + version);

// Type safety example
Auto x = 10;
Auto y = 20;
Auto sum = x + y;                 // Number addition

Auto greeting = "Hello ";
Auto subject = "World";
Auto message = greeting + subject; // String concatenation

Display("Sum: " + sum);
Display("Message: " + message);