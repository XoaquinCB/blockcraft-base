#ifndef BT_COMMANDS_H
#define BT_COMMANDS_H

/**
 * Handles any incoming bluetooth commands.
 */
void bt_commands_update_rx();

/**
 * Sends the current block structure over bluetooth.
 */
void bt_commands_send_current_structure();

#endif /* BT_COMAMNDS_H */