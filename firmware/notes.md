# New functionality

## Clock

- ARP and SEQ modes will depend on a clock rate, so I'll have to figure that out first
- Notably, they will depend on divisions (or multiplications) of a clock. That clock could be sourced externally (via sync)
- A "rate" or "tempo" knob will let the user set the clock speed when there is no external sync input
    - If there is an external sync input, the rate knob won't do anything
- A time division knob will let the user choose a division of the clock rate
- I will have to decide what QQPN to use, or to support multiple and make it configurable (via DIP switches?)
- First intuition is to use a timer
    - Can't set gate high and low in same timer, would be too fast. Would probably need two timer pulses per note:
        - One to bring gate high and select current not
        - Next one to bring gate low
    - Time between the two timers determines gate width. Will hard code this to 50% for now, so if the clo/k rate is 10 notes per second, we actually run the timer routine 20 times a second.
    - The timer route will need to:
        - Check gate state
            - If up, bring down
            - If down, bring up, continue
        - Determine which note to play
        - Set CV out
    - I can keep a pointer to the next note in the sequence and change it on each gate down
    - Timer shouldn't need to care about current key state or button presses, should only need to worry about the sequence list
        - Could even abstract that away with a `get_next_note` function.
- How to handle external sync signal?
    - The KORG Volca Bass uses an integrator to split the external clock pulse in to a SYNC_RISE and SYNC_FALL. I like this as it makes it easy to determine exactly when a falling edge happens
    - The mutable instruments anrushri might be a good place to look for examples:
        - Schematics: https://mutable-instruments.net/archive/anushri/build/
        - Code: https://github.com/pichenettes/anushri


## Note Sequence

- Should be able to use the same note sequence structure for both arp and seq
- Will probably want a lock to prevent problems when the notes are being added/removed to the sequence and read by the timer
- Will need to support adding keys in the following orders at first:
    - Last key pressed (key press stack)
    - In order (low to hight)
    - In order (high to low)
- API could look something like this:
    - `void seq_push_key(struct sequence*, uint8_t note, uint8_t mode)`
    - `void seq_pop_key(struct sequence*, uint8_t note, uint8_t mode)`
    - `uint8_t seq_get_next(struct sequence*, uint8_T mode)`
        - Could have this automatically increment the `next_key` pointer if mode is arp or seq. Not sure yet if I'll want that to be a side effect or explicit




## Arp

### First Version

- Arpegiator mode will start/stop using the play/pause button
    - First press puts it in to arp mode
    - Next press takes it out of arp mode
- Notes that are current held down are played in the order that they were pressed
- Rate at which notes are played is determined by rate knob
    - Will set up 4051 demultiplexer so that I can use just one ADC for the thre
