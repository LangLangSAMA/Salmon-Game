Name: Lang Cheng
Student Number: 40588155
CSID: q9o0b
CWL: langcl

Previously Done:
Salmon can be controlled to move by pressing arrow keys or WASD.
Debug mode activated by pressing C.
Press V to activate or deactivate the expected collision mode, which will show the collision point on the border and it depends 
on the salmon current movement direction.
After pressing Z, it activates the accurate collision check mode, the new spawned fish collision is now checked by 
using 8 different points around the fish based on its bounding box for better accuracy.

Particle Animation:
Spawn pebbles with random inital direction and velocities, and add gravtiy to each of them.

Bounding Pebbles:
Add collision between pebbles and salmon/fish/turtles/other pebbles. Pebble collision is handled firstly by separating them, then do the
actual collision, since they might stick together when they spawn at the same point.

Creative Part:
Including force of water currents in pebble motion computation.
Press T to toggle the water force, and use shift + , or shift + . to change the magnitude.