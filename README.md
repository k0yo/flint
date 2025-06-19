# Flint

**Flint** is a clean, beginner-friendly programming language inspired by Python. It simplifies syntax by removing unnecessary brackets and refining grammar, making code easier to read and write. [Read the docs.](docs.md)

---

### 🔹 Example

```flint
;- Flint Café sample with all features -;
; This is a single-line comment
start:
    write "☕ Welcome to Flint Café! ☕"
    ask "What is your name, customer?" as customer_name

    object Customer:
        name = ""
        balance = 20
        order = ""
        mood = "neutral"
        loyalty = false

    let guest = Customer()
    guest.name = customer_name

    menu = {"coffee": 5, "tea": 3, "cake": 7}
    write "Today's menu:"
    write "Coffee: $${menu[\\"coffee\\"]}, Tea: $${menu[\\"tea\\"]}, Cake: $${menu[\\"cake\\"]}"

    ask "What would you like to order? (coffee/tea/cake)" as guest.order
    price = menu[guest.order]
    if guest.balance >= price:
        guest.balance -= price
        write "Enjoy your ${guest.order}, ${guest.name}! Remaining balance: $${guest.balance}"
    else:
        write "Sorry, you don't have enough money for ${guest.order}."

    ask "How do you feel about your order?" as guest.mood
    check guest.mood:
        equals "happy":
            write "We're glad you're happy!"
        equals "disappointed":
            write "Sorry to hear that. Next time will be better!"
        equals "excited":
            write "Excitement is contagious!"
        equals "bored":
            write "We'll try to spice things up!"

    guest.loyalty = guest.balance < 10 or guest.mood == "happy"
    if guest.loyalty:
        write "You've joined our loyalty program! Free cookie next time."
    else:
        write "Earn loyalty by visiting more or sharing your happiness!"

    favorites = ["coffee", "tea", "cake"]
    i = 0
    loop 3:
        write "Customer favorite #${i+1}: ${favorites[i]}"
        i++

    command shout item:
        write upper item

    shout "thank you for visiting!"

    random.seed 7
    lucky_number = random.int 1 100
    write "Your lucky number for today is: ${lucky_number}"
    wait 1

    review = "  Great service!  "
    clean_review = trim review
    write "Customer review: '${clean_review}'"
    write "Reversed: ${reverse clean_review}"

    guest.balance += 5
    guest.balance--
    write "You found $5! New balance: $${guest.balance}"

    write "Goodbye, ${guest.name}! Come back soon to Flint Café."
```

---

Flint is easy to learn, visually uncluttered, and powerful enough for real-world programming.
