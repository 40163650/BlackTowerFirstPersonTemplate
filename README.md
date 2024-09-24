I chose part B of the test

I added a "blink" skill to the game, similar to the one from Dishonored. The player can hold down the B key, as indicated by the UI, then a ball will appear where they are looking. This was done by casting a ray from the camera in the direction it's facing and seeing what it collides with. The ball had to be excluded from the collision tests as the ray would hit it first and the ball would slowly move towards the camera.

Upon release of the B key, the screen will briefly fade to black and the player will be teleported to the spot indicated by the ball, then the black will fade away, as if they had blinked.

At this point a cooldown timer starts, the player cannot blink again until the cooldown timer has finished. It lasts for 2 seconds. The player can see how long they have left to be able to teleport again by looking at the animated HUD element.

A sound effect will play when the player teleports, and when the cool down finishes.

The controls are WASD to move around, Left Shift to sprint, space to jump, and B to blink / teleport.
