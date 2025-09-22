// Functions in Rubber Duck
fun greet(name) {
    return "Hello, " + name + "!";
}

fun add(a, b) {
    return a + b;
}

fun factorial(n) {
    if (n <= 1) {
        return 1;
    }
    return n * factorial(n - 1);
}

// Using the functions
Auto message = greet("Rubber Duck");
Display(message);

Auto sum = add(5, 3);
Display("5 + 3 = " + sum);

Auto fact = factorial(5);
Display("5! = " + fact);