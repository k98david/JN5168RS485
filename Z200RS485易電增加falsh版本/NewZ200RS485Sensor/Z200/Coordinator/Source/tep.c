/*
 * tep.c
 *
 *  Created on: 2016/10/25
 *      Author: hodge
 */


#if 0


PUBLIC void WUART_vTick_Old(void)
{
	/* Paired ? */
	if (WUART_bStatePaired(WUART_sData.u8State))
	{
		bool_t bLowReached;
		bool_t bHighReached;

		/* Read the low and high levels reached from the uart transmit queue */
		bLowReached  = QUEUE_bLowReached(&asQueue[WUART_QUEUE_TX]);
		bHighReached = QUEUE_bHighReached(&asQueue[WUART_QUEUE_TX]);

		/* Reached a low amount of free space in the UART's transmit queue ? */
		if (bLowReached)
		{
			/* Set our status to indicate have changed our radio receive state */
			U8_SET_BITS(&WUART_sData.u8Status, WUART_STATUS_RX_CHANGE);
			/* clear our status to indicate we can't receive over the radio */
			U8_CLR_BITS(&WUART_sData.u8Status, WUART_STATUS_RX_ENABLED);
		}
		/* Reached a high amount of free space in the UART's transmit queue ? */
		else if (bHighReached)
		{
			/* Set our status to indicate have changed our radio receive state */
			U8_SET_BITS(&WUART_sData.u8Status, WUART_STATUS_RX_CHANGE);
			/* Set our status to indicate we can receive over the radio */
			U8_SET_BITS(&WUART_sData.u8Status, WUART_STATUS_RX_ENABLED);
		}
	}

	/* Has the state timer expired ? */
	if (NODE_sData.u32Timer - WUART_sData.u32StateStart > WUART_sData.u32StateDuration)
	{
		/* Invalidate the duration to prevent further expiries on future calls */
		WUART_sData.u32StateDuration = 0xffffffff;

		/* Debug */
		DBG_vPrintf(WUART_TRACE, "\r\n%d WUART> WUART_vTick() Timeout state=%d", NODE_sData.u32Timer, WUART_sData.u8State);

		/* What state has expired ? */
		switch (WUART_sData.u8State)
		{
			/* None ? */
			case WUART_STATE_NONE:
		  	{
				/* Do nothing - this state shouldn't time out */
				;
			} break;

			/* Idle ? */
			case WUART_STATE_IDLE:
		  	{
		  		/* Set sequence */
		  		au8CmdIdle[WUART_MSG_SEQ_TX] = WUART_u8MsgSeqTxNew();
		  		au8CmdIdle[WUART_MSG_SEQ_RX] = ' ';
				/* Re-enter idle state, broadcast idle request */
				WUART_vState(WUART_STATE_IDLE);
				WUART_vTx((uint64) ZPS_E_BROADCAST_RX_ON, u8CmdIdle, au8CmdIdle);
			} break;

			/* Dial ? */
			/* Dial connect ? */
			/* Answer ? */
			/* Answer connect ? */
			case WUART_STATE_DIAL:
			case WUART_STATE_DCONNECT:
			case WUART_STATE_ANSWER:
			case WUART_STATE_ACONNECT:
		  	{
		  		/* Set sequence */
		  		au8CmdError[WUART_MSG_SEQ_TX] = WUART_u8MsgSeqTxNew();
		  		au8CmdError[WUART_MSG_SEQ_RX] = ' ';
				/* Go to idle state, transmit error */
				WUART_vState(WUART_STATE_IDLE);
				WUART_vTx(WUART_sData.u64Online, u8CmdError, au8CmdError);
			} break;

			/* Online state ? */
			case WUART_STATE_ONLINE:
		  	{
				/* Set ping status */
				U8_SET_BITS(&WUART_sData.u8Status, WUART_STATUS_PING);
			} break;

			/* Ack state ? */
			case WUART_STATE_ACK:
		  	{
		  		/* OK to retry ? */
		  		if (WUART_sData.u8Retry <= 5)
		  		{
					/* Increment retires */
					WUART_sData.u8Retry++;
		  			/* Go to online state no transmissison */
					WUART_vState(WUART_STATE_ONLINE);
		  		}
		  		else
		  		{
			  		/* Set sequence */
			  		au8CmdError[WUART_MSG_SEQ_TX] = WUART_u8MsgSeqTxNew();
			  		au8CmdError[WUART_MSG_SEQ_RX] = ' ';
					/* Go to idle state, transmit error */
					WUART_vState(WUART_STATE_IDLE);
					WUART_vTx(WUART_sData.u64Online, u8CmdError, au8CmdError);
				}
			} break;

			/* Others ? */
			default:
			{
				/* Do nothing - need to make sure we don't get stuck */
				;
			} break;
		}
	}

	/* Are we online ? */
	if (WUART_sData.u8State == WUART_STATE_ONLINE)
	{
		/* Start transmit from uart if appropriate */
		UART_vStartTx(WUART_sData.u8Uart);
		/* Send data packet if appropriate */
		//WUART_bTxData();
	}

	/* What state are we in now ? */
	switch (WUART_sData.u8State)
	{
		/* None (not in network) ? */
		case WUART_STATE_NONE:
		{
			/* Flash LEDs alternating */
			vLedControl(0, (NODE_sData.u32Timer & 0x20) ? TRUE  : FALSE);
			vLedControl(1, (NODE_sData.u32Timer & 0x20) ? FALSE : TRUE);
		} break;
		/* Online (paired) ? */
		case WUART_STATE_ONLINE:
		{
			/* Set LEDs according to UART enabled states */
			vLedControl(0, UART_bTxEnabled(WUART_sData.u8Uart));
			vLedControl(1, UART_bRxEnabled(WUART_sData.u8Uart));
		} break;
		/* Ack (paired) ? */
		case WUART_STATE_ACK:
		{
			/* Set LEDs according to UART enabled states */
			vLedControl(0, FALSE);
			vLedControl(1, UART_bRxEnabled(WUART_sData.u8Uart));
		} break;
		/* Idle (unpaired) ? */
		case WUART_STATE_IDLE:
		{
			/* Flash LEDs slowly together */
			vLedControl(0, (NODE_sData.u32Timer & 0x40) ? TRUE : FALSE);
			vLedControl(1, (NODE_sData.u32Timer & 0x40) ? TRUE : FALSE);
		} break;
		/* Others (pairing in progress) ? */
		default:
		{
			/* Flash LEDs quickly together */
			vLedControl(0, (NODE_sData.u32Timer & 0x20) ? TRUE : FALSE);
			vLedControl(1, (NODE_sData.u32Timer & 0x20) ? TRUE : FALSE);
		} break;
	}

	/* Queue stats ? */
	#if QUEUE_STATS
	{
		/* Update timers for queues */
		QUEUE_vTick(&asQueue[WUART_QUEUE_TX]);
		QUEUE_vTick(&asQueue[WUART_QUEUE_RX]);
	}
	#endif

	/* LCD ? */
	#if WUART_LCD
	/* Approx two seconds and a half ? */
	if ((NODE_sData.u32Timer & 0xff) == 0)
	{
		/* Display values */
		LCD_vLeft( 0, (uint32) UART_bRxEnabled(WUART_sData.u8Uart), 1);
		LCD_vRight(0, (uint32) UART_bTxEnabled(WUART_sData.u8Uart), 1);
		#if UART_STATS
		/* Display values */
		LCD_vLeft( 1, UART_u32Rxed(WUART_sData.u8Uart), 6);
		LCD_vRight(1, UART_u32Txed(WUART_sData.u8Uart), 6);
		LCD_vLeft( 2, UART_u32RxDisabled(WUART_sData.u8Uart), 6);
		LCD_vRight(2, UART_u32TxDisabled(WUART_sData.u8Uart), 6);
		#endif
		#if QUEUE_STATS
		/* Display values */
		LCD_vLeft( 3, QUEUE_u16Used(&asQueue[WUART_QUEUE_RX]), 5);
		LCD_vRight(3, QUEUE_u16Used(&asQueue[WUART_QUEUE_TX]), 5);
		LCD_vLeft( 4, QUEUE_u16Free(&asQueue[WUART_QUEUE_RX]), 5);
		LCD_vRight(4, QUEUE_u16Free(&asQueue[WUART_QUEUE_TX]), 5);
//		LCD_vLeft( 3, QUEUE_u32Put(&asQueue[WUART_QUEUE_RX]), 6);
//		LCD_vRight(3, QUEUE_u32Put(&asQueue[WUART_QUEUE_TX]), 6);
//		LCD_vLeft( 4, QUEUE_u32Got(&asQueue[WUART_QUEUE_RX]), 6);
//		LCD_vRight(4, QUEUE_u32Got(&asQueue[WUART_QUEUE_TX]), 6);
		LCD_vLeft( 5, QUEUE_u32Rate(&asQueue[WUART_QUEUE_RX]), 6);
		LCD_vRight(5, QUEUE_u32Rate(&asQueue[WUART_QUEUE_TX]), 6);
		#endif
		#if WUART_STATS
		/* Display values */
		LCD_vLeft( 6, WUART_sData.u32Txed, 6);
		LCD_vRight(6, WUART_sData.u32Rxed, 6);
		LCD_vLeft( 7, WUART_sData.u32TxPackets, 6);
		LCD_vRight(7, WUART_sData.u32RxPackets, 6);
		#endif
		/* Draw the display */
		LCD_vDraw();
	}
	#endif
}


PUBLIC void WUART_vRx(uint64 u64RxAddr, uint16 u16RxSize, uint8 *pu8RxPayload)
{
	bool_t   bHandled = FALSE;	/**< Assume we won't handle the message */

	/* Debug */
	DBG_vPrintf(WUART_TRACE, "\r\n%d WUART< QQWUART_vRx(%x:%x %d %s)",
		NODE_sData.u32Timer,
		(uint32)(u64RxAddr >> 32),
		(uint32)(u64RxAddr & 0xffffffff),
		u16RxSize,
		pu8RxPayload+8);

	/* Not in none state */
	if (WUART_sData.u8State != WUART_STATE_NONE)
	{
		DBG_vPrintf(WUART_TRACE,"=1=\r\n");
		/* Well formed message ? */
		if (u16RxSize                 >= 4 &&
		    pu8RxPayload[WUART_MSG_START] == au8CmdData[WUART_MSG_START] &&
		    pu8RxPayload[u16RxSize-1]     == '\0')
		{
			DBG_vPrintf(WUART_TRACE,"=2=\r\n");
			/* Is this a repeated message from the node we are online with ? */
			if (WUART_sData.u64Online  == u64RxAddr &&
			    WUART_sData.u8MsgSeqRx == pu8RxPayload[WUART_MSG_SEQ_TX])
			{
				/* Treat as handled to ignore it */
				bHandled = TRUE;
			}
			/* Not a repeated message from node we are online with ? */
			else
			{
				DBG_vPrintf(WUART_TRACE,"=3=\r\n");
				/* Data command ? */
				if (pu8RxPayload[WUART_MSG_CMD] == au8CmdData[WUART_MSG_CMD])
				{
					/* In answer connect, online or ack state ? */
					if (WUART_sData.u8State == WUART_STATE_ACONNECT ||
						WUART_sData.u8State == WUART_STATE_ONLINE   ||
						WUART_sData.u8State == WUART_STATE_ACK)
					{
						/* From node we are online with ? */
						if (u64RxAddr == WUART_sData.u64Online)
						{
							bool_t bRxOther;
							bool_t bTxOther;

							/* Just coming online following an answer connect ? */
							if (WUART_sData.u8State == WUART_STATE_ACONNECT)
							{
								/* Clear status and ack flags */
								WUART_sData.u8Status = 0;
								WUART_sData.u8Ack    = 0;
								/* Set our status to indicate have changed our radio receive state */
								U8_SET_BITS(&WUART_sData.u8Status, WUART_STATUS_RX_CHANGE);
								/* Is the UART's transmit buffer not low on space ? */
								if (! QUEUE_bLowState(&asQueue[WUART_QUEUE_TX]))
								{
									/* Set our status to indicate we can receive over the radio */
									U8_SET_BITS(&WUART_sData.u8Status, WUART_STATUS_RX_ENABLED);
								}
								/* Go to online state no transmissison */
								WUART_vState(WUART_STATE_ONLINE);
							}

							/* Does packet have some acks set ? */
							if (pu8RxPayload[WUART_DATA_ACK] & 0xF)
							{
								/* Is this the ack for the packet we previously sent ? */
								if (pu8RxPayload[WUART_MSG_SEQ_RX] == au8CmdData[WUART_MSG_SEQ_TX])
								{
									/* Ack for data ? */
									if (pu8RxPayload[WUART_DATA_ACK] & WUART_ACK_DATA_ACK)
									{
										/* Reset data length so we can build the next packet */
										u8CmdData = WUART_DATA_DATA;
										/* Invalidate sequence */
										au8CmdData[WUART_DATA_SEQ]	 = '\0';
									}
									/* Nak for data ? */
									if (pu8RxPayload[WUART_DATA_ACK] & WUART_ACK_DATA_NAK)
									{
										/* Do nothing we should be told to turn off tx */
										;
									}
									/* Ack for rx change ? */
									if (pu8RxPayload[WUART_DATA_ACK] & WUART_ACK_RX_CHANGE)
									{
										/* Clear rx change status bit */
										U8_CLR_BITS(&WUART_sData.u8Status, WUART_STATUS_RX_CHANGE);
									}
									/* Ack for ping ? */
									if (pu8RxPayload[WUART_DATA_ACK] & WUART_ACK_PING)
									{
										/* Clear ping status bit */
										U8_CLR_BITS(&WUART_sData.u8Status, WUART_STATUS_PING);
									}
									/* Zero retires */
									WUART_sData.u8Retry = 0;
									/* Waiting for this ack ? */
									if (WUART_sData.u8State == WUART_STATE_ACK)
									{
										/* Go to online state no transmissison */
										WUART_vState(WUART_STATE_ONLINE);
									}
								}
							}

							/* Extract enabled flags from other device's status */
							bRxOther = (pu8RxPayload[WUART_DATA_STATUS] & WUART_STATUS_RX_ENABLED);
							bTxOther = (pu8RxPayload[WUART_DATA_STATUS] & WUART_STATUS_TX_ENABLED);

							/* Does the other device's receive status not match our transmit status ? */
							if (bRxOther != WUART_bTxEnabled())
							{
								/* Turn on ? */
								if (bRxOther)
								{
									/* Set our transmit status */
									U8_SET_BITS(&WUART_sData.u8Status, WUART_STATUS_TX_ENABLED);
								}
								else
								{
									/* Clear our transmit status */
									U8_CLR_BITS(&WUART_sData.u8Status, WUART_STATUS_TX_ENABLED);
								}
							}

							/* Does the other devices transmit status not match our receive status ? */
							if (bTxOther != WUART_bRxEnabled())
							{
								/* Set our receive change status to force an update */
								U8_SET_BITS(&WUART_sData.u8Status, WUART_STATUS_RX_CHANGE);
							}

							/* Is the other device telling us about an rx status change ? */
							if (pu8RxPayload[WUART_DATA_STATUS] & WUART_STATUS_RX_CHANGE)
							{
								/* Set our receive change ack to send an ack */
								U8_SET_BITS(&WUART_sData.u8Ack, WUART_ACK_RX_CHANGE);
							}

							/* Is the other device pinging us ? */
							if (pu8RxPayload[WUART_DATA_STATUS] & WUART_STATUS_PING)
							{
								/* Set our ping ack to send an ack */
								U8_SET_BITS(&WUART_sData.u8Ack, WUART_ACK_PING);
							}

							/* Any data in the message ? */
							if (u16RxSize > WUART_DATA_DATA && pu8RxPayload[WUART_DATA_SEQ] != '\0')
							{
								/* Not repeated data ? */
								if (WUART_sData.u8DataSeqRx != pu8RxPayload[WUART_DATA_SEQ])
								{
									/* Stats ? */
									#if WUART_STATS
									{
										/* Increment received data packets */
										WUART_sData.u32RxPackets++;
										/* Update received characters */
										WUART_sData.u32Rxed += (u16RxSize - WUART_DATA_DATA - 1);
									}
									#endif
									/* Is there space in the UART transmit buffer for this data */
									if (QUEUE_u16Free(&asQueue[WUART_QUEUE_TX]) >= u16RxSize - WUART_DATA_DATA - 1)
									{
										uint16 u16Pos;

										/* Loop through received data */
										for (u16Pos = WUART_DATA_DATA; u16Pos < u16RxSize-1; u16Pos++)
										{
											/* Get uart to transmit this byte */
											UART_bPutChar(WUART_sData.u8Uart, pu8RxPayload[u16Pos]);
										}
										/* Start transmit */
										UART_vStartTx(WUART_sData.u8Uart);

										/* Need to ack it now we've received it */
										U8_SET_BITS(&WUART_sData.u8Ack, WUART_ACK_DATA_ACK);
										U8_CLR_BITS(&WUART_sData.u8Ack, WUART_ACK_DATA_NAK);

										/* Remember this sequence so we don't add it to the output queue again */
										WUART_sData.u8DataSeqRx = pu8RxPayload[WUART_DATA_SEQ];
									}
									/* Not enough room for data in transmit queue ? */
									else
									{
										/* Need to nak it so it gets resent later when we have space */
										U8_CLR_BITS(&WUART_sData.u8Ack, WUART_ACK_DATA_ACK);
										U8_SET_BITS(&WUART_sData.u8Ack, WUART_ACK_DATA_NAK);
									}
								}
								/* Old previously received data ? */
								else
								{
									/* Need to ack it so it is not sent again */
									U8_SET_BITS(&WUART_sData.u8Ack, WUART_ACK_DATA_ACK);
									U8_CLR_BITS(&WUART_sData.u8Ack, WUART_ACK_DATA_NAK);
								}
							}
							/* Acks to be sent ? */
							if (WUART_sData.u8Ack != 0)
							{
								/* Set received sequence */
								au8CmdData[WUART_MSG_SEQ_RX] = pu8RxPayload[WUART_MSG_SEQ_TX];
							}
							else
							{
								/* Clear received sequence */
								au8CmdData[WUART_MSG_SEQ_RX] = ' ';
							}
							/* Are we online ? */
							if (WUART_sData.u8State == WUART_STATE_ONLINE)
							{
								bool_t bLowReached;
								bool_t bHighReached;

								/* Read the low and high levels reached from the uart transmit queue */
								bLowReached  = QUEUE_bLowReached(&asQueue[WUART_QUEUE_TX]);
								bHighReached = QUEUE_bHighReached(&asQueue[WUART_QUEUE_TX]);

								/* Reached a low amount of free space in the UART's transmit queue ? */
								if (bLowReached)
								{
									/* Set our status to indicate have changed our radio receive state */
									U8_SET_BITS(&WUART_sData.u8Status, WUART_STATUS_RX_CHANGE);
									/* clear our status to indicate we can't receive over the radio */
									U8_CLR_BITS(&WUART_sData.u8Status, WUART_STATUS_RX_ENABLED);
								}
								/* Reached a high amount of free space in the UART's transmit queue ? */
								else if (bHighReached)
								{
									/* Set our status to indicate have changed our radio receive state */
									U8_SET_BITS(&WUART_sData.u8Status, WUART_STATUS_RX_CHANGE);
									/* Set our status to indicate we can receive over the radio */
									U8_SET_BITS(&WUART_sData.u8Status, WUART_STATUS_RX_ENABLED);
								}

								/* Send data packet if appropriate */
								//WUART_bTxData();
							}
							/* Always handled */
							bHandled = TRUE;
						}
					}
				}

				/* Idle command ? */
				else if (pu8RxPayload[WUART_MSG_CMD] == au8CmdIdle[WUART_MSG_CMD])
				{
					/* Set sequence */
					au8RepIdle[WUART_MSG_SEQ_TX] = WUART_u8MsgSeqTxNew();
					au8RepIdle[WUART_MSG_SEQ_RX] = pu8RxPayload[WUART_MSG_SEQ_TX];
					/* From node we are online with ? */
					if (u64RxAddr == WUART_sData.u64Online)
					{
						/* Unexpected - drop back to idle state transmit response */
						WUART_vState(WUART_STATE_IDLE);
					}
					/* Transmit idle response */
					WUART_vTx(u64RxAddr, u8RepIdle, au8RepIdle);
					/* Always consider idle request handled */
					bHandled = TRUE;
				}

				/* Ring command ? */
				else if (pu8RxPayload[WUART_MSG_CMD] == au8CmdRing[WUART_MSG_CMD])
				{
					/* In idle state ? */
					if (WUART_sData.u8State == WUART_STATE_IDLE)
					{
						/*	Note this nodes address while we try to go online with it */
						WUART_sData.u64Online = u64RxAddr;
						/* Set sequence */
						au8RepAnswer[WUART_MSG_SEQ_TX] = WUART_u8MsgSeqTxNew();
						au8RepAnswer[WUART_MSG_SEQ_RX] = pu8RxPayload[WUART_MSG_SEQ_TX];
						/* Enter answer state transmit answer response */
						WUART_vState(WUART_STATE_ANSWER);
						WUART_vTx(u64RxAddr, u8RepAnswer, au8RepAnswer);
						/* Handled */
						bHandled = TRUE;
					}
					/* From node we are online with ? */
					else if (u64RxAddr == WUART_sData.u64Online)
					{
						/* In answer state ? */
						if (WUART_sData.u8State == WUART_STATE_ANSWER)
						{
							/* Set sequence */
							au8RepAnswer[WUART_MSG_SEQ_TX] = WUART_u8MsgSeqTxNew();
							au8RepAnswer[WUART_MSG_SEQ_RX] = pu8RxPayload[WUART_MSG_SEQ_TX];
							/* Re-enter answer state transmit answer response */
							WUART_vState(WUART_STATE_ANSWER);
							WUART_vTx(u64RxAddr, u8RepAnswer, au8RepAnswer);
							/* Handled */
							bHandled = TRUE;
						}
					}
				}

				/* Connect command ? */
				else if (pu8RxPayload[WUART_MSG_CMD] == au8CmdConnect[WUART_MSG_CMD])
				{
					/* In answer or answer connect state ? */
					if (WUART_sData.u8State == WUART_STATE_ANSWER  ||
						WUART_sData.u8State == WUART_STATE_ACONNECT)
					{
						/* From node we are online with ? */
						if (u64RxAddr == WUART_sData.u64Online)
						{
							/* Set sequence */
							au8RepConnected[WUART_MSG_SEQ_TX] = WUART_u8MsgSeqTxNew();
							au8RepConnected[WUART_MSG_SEQ_RX] = pu8RxPayload[WUART_MSG_SEQ_TX];
							/* Enter answer connect state transmit connect response */
							WUART_vState(WUART_STATE_ACONNECT);
							WUART_vTx(u64RxAddr, u8RepConnected, au8RepConnected);
							/* Handled */
							bHandled = TRUE;
						}
					}
				}

				/* Idle Reply ? */
				else if (pu8RxPayload[WUART_MSG_CMD] == au8RepIdle[WUART_MSG_CMD])
				{
					/* In idle state ? */
					if (WUART_sData.u8State == WUART_STATE_IDLE)
					{
						/*	Note this nodes address while we try to go online with it */
						WUART_sData.u64Online = u64RxAddr;
						/* Set sequence */
						au8CmdRing[WUART_MSG_SEQ_TX] = WUART_u8MsgSeqTxNew();
						au8CmdRing[WUART_MSG_SEQ_RX] = pu8RxPayload[WUART_MSG_SEQ_TX];
						/* Enter dial state transmit ring request */
						WUART_vState(WUART_STATE_DIAL);
						WUART_vTx(u64RxAddr, u8CmdRing, au8CmdRing);
					}
					/* From node we are online with ? */
					else if (u64RxAddr == WUART_sData.u64Online)
					{
						/* In dial state */
						if (WUART_sData.u8State == WUART_STATE_DIAL)
						{
							/* Set sequence */
							au8CmdRing[WUART_MSG_SEQ_TX] = WUART_u8MsgSeqTxNew();
							au8CmdRing[WUART_MSG_SEQ_RX] = pu8RxPayload[WUART_MSG_SEQ_TX];
							/* Re-enter dial state transmit ring request */
							WUART_vState(WUART_STATE_DIAL);
							WUART_vTx(u64RxAddr, u8CmdRing, au8CmdRing);
						}
						else
						{
							/* Unexpected - drop back to idle state */
							WUART_vState(WUART_STATE_IDLE);
						}
					}
					/* Always consider idle request handled */
					bHandled = TRUE;
				}

				/* Answer reply ? */
				else if (pu8RxPayload[WUART_MSG_CMD] == au8RepAnswer[WUART_MSG_CMD])
				{
					/* In dial or dial connect state */
					if (WUART_sData.u8State == WUART_STATE_DIAL    ||
						WUART_sData.u8State == WUART_STATE_DCONNECT)
					{
						/* From node we are online with ? */
						if (u64RxAddr == WUART_sData.u64Online)
						{
							/* Set sequence */
							au8CmdConnect[WUART_MSG_SEQ_TX] = WUART_u8MsgSeqTxNew();
							au8CmdConnect[WUART_MSG_SEQ_RX] = pu8RxPayload[WUART_MSG_SEQ_TX];
							/* Enter dial connect state transmit connect request */
							WUART_vState(WUART_STATE_DCONNECT);
							WUART_vTx(u64RxAddr, u8CmdConnect, au8CmdConnect);
							/* Handled */
							bHandled = TRUE;
						}
					}
				}

				/* Connected reply ? */
				else if (pu8RxPayload[WUART_MSG_CMD] == au8RepConnected[WUART_MSG_CMD])
				{
					/* In dial connect or online state */
					if (WUART_sData.u8State == WUART_STATE_DCONNECT    ||
						WUART_sData.u8State == WUART_STATE_ONLINE)
					{
						/* From node we are online with ? */
						if (u64RxAddr == WUART_sData.u64Online)
						{
							/* Clear status and ack flags */
							WUART_sData.u8Status = 0;
							WUART_sData.u8Ack    = 0;
							/* Set our status to indicate have changed our radio receive state */
							U8_SET_BITS(&WUART_sData.u8Status, WUART_STATUS_RX_CHANGE);
							/* Is the UART's transmit buffer not low on space ? */
							if (! QUEUE_bLowState(&asQueue[WUART_QUEUE_TX]))
							{
								/* Set our status to indicate we can receive over the radio */
								U8_SET_BITS(&WUART_sData.u8Status, WUART_STATUS_RX_ENABLED);
							}
							/* Waiting to connect ? */
							if (WUART_sData.u8State == WUART_STATE_DCONNECT)
							{
								/* Go to online state no transmissison */
								WUART_vState(WUART_STATE_ONLINE);
							}
							/* Set received sequence in data message that should go out soon */
							au8CmdData[WUART_MSG_SEQ_RX] = pu8RxPayload[WUART_MSG_SEQ_TX];
							/* Handled */
							bHandled = TRUE;
						}
					}
				}

				/* Error command ? */
				else if (pu8RxPayload[WUART_MSG_CMD] == au8CmdError[WUART_MSG_CMD])
				{
					/* Not in idle state ? */
					if (WUART_sData.u8State != WUART_STATE_IDLE)
					{
						/* From node we are online with ? */
						if (u64RxAddr == WUART_sData.u64Online)
						{
							/* Return to idle state no transmit */
							WUART_vState(WUART_STATE_IDLE);
						}
					}
					/* Always handled */
					bHandled = TRUE;
				}

				/* From node we are online with ? */
				if (WUART_sData.u64Online  == u64RxAddr)
				{
					/* Note the sequence number */
					WUART_sData.u8MsgSeqRx = pu8RxPayload[WUART_MSG_SEQ_TX];
				}
			}
		}

		/* Didn't handle message above ? */
		if (bHandled == FALSE)
		{
			/* Set sequence */
			au8CmdError[WUART_MSG_SEQ_TX] = WUART_u8MsgSeqTxNew();
			au8CmdError[WUART_MSG_SEQ_RX] = pu8RxPayload[WUART_MSG_SEQ_TX];;
			/* Not in idle state and from node we are online with ? */
			if (WUART_sData.u8State != WUART_STATE_IDLE && u64RxAddr == WUART_sData.u64Online)
			{
				/* Return to idle state transmit error */
				WUART_vState(WUART_STATE_IDLE);
			}
			/* Transmit error response */
			WUART_vTx(u64RxAddr, u8CmdError, au8CmdError);
		}
	}
}


PRIVATE bool_t WUART_bTxData_Old(void)
{
	bool_t bTxData  = FALSE;
	uint8 u8TxState = WUART_STATE_NONE;

	/* In online state ? */
	if (1)
	//if (WUART_sData.u8State == WUART_STATE_ONLINE)
	{
		/* Data transmit packet is empty and
		   and we are allowed to transmit and
		   we've received data from the UART ? */
		if (u8CmdData 								==  WUART_DATA_DATA &&
		   (WUART_sData.u8Status & WUART_STATUS_TX_ENABLED) != 0		&&
			QUEUE_bEmpty(&asQueue[WUART_QUEUE_RX])          == FALSE)
		{
			uint8 u8Char;
			bool_t bGetChar = TRUE;

			/* Set data sequence */
			au8CmdData[WUART_DATA_SEQ] = WUART_u8DataSeqTxNew();
			/* Loop while we can add data from UART */
			while(QUEUE_bEmpty(&asQueue[WUART_QUEUE_RX]) 	== FALSE	&&
				  bGetChar               	== TRUE 	&&
				 u8CmdData                  	<  NODE_PAYLOAD-1)
			{
				/* Attempt to get character from UART receive buffer */
				bGetChar = UART_bGetChar(WUART_sData.u8Uart, (char *) &u8Char);
				/* Got character - add to message */
				if (bGetChar) au8CmdData[u8CmdData++] = u8Char;
			}

			/* Append null */
			au8CmdData[u8CmdData++] = '\0';

			/* Stats ? */
			#if WUART_STATS
			{
				/* Increment transmitted data packets */
				WUART_sData.u32TxPackets++;
				/* Update transmitted characters */
				WUART_sData.u32Txed += (u8CmdData - WUART_DATA_DATA - 1);
			}
			#endif
		}

		/* Data transmit packet is not empty and
		   and we are allowed to transmit */
		if (u8CmdData 										   >  WUART_DATA_DATA &&
		   (WUART_sData.u8Status & WUART_STATUS_TX_ENABLED) != 0)
		{
			/* Go to ACK state */
			u8TxState = WUART_STATE_ACK;
			/* Note we will transmit */
			bTxData = TRUE;
		}
		/* Has our status changed ? */
		else if ((WUART_sData.u8Status & (WUART_STATUS_RX_CHANGE | WUART_STATUS_PING)) != 0)
		{
			/* Go to ACK state */
			u8TxState =  WUART_STATE_ACK;
			/* Note we will transmit */
			bTxData = TRUE;
		}
		/* We want to send an ack only ? */
		else if (WUART_sData.u8Ack != 0)
		{
			/* Go to ONLINE state */
			u8TxState = WUART_STATE_ONLINE;
			/* Note we will transmit */
			bTxData = TRUE;
		}

		/* Want to transmit ? */
		if (bTxData)
		{
			/* Debug */
			DBG_vPrintf(WUART_TRACE, "\r\n%d WUART< WUART_vTxData()", NODE_sData.u32Timer);
			/* Set message sequence */
			au8CmdData[WUART_MSG_SEQ_TX] = WUART_u8MsgSeqTxNew();
			/* Set current status, ack values for this packet */
			au8CmdData[WUART_DATA_STATUS] = WUART_sData.u8Status + '0';
			au8CmdData[WUART_DATA_ACK] 	  = WUART_sData.u8Ack    + '0';
			/* Clear our outstanding acks if the ack fails will get a resend anyway */
			WUART_sData.u8Ack = 0;
			/* Change to state and transmit */
			WUART_vState(u8TxState);
			WUART_sData.u64Online=0x8812;
			WUART_vTx(WUART_sData.u64Online, u8CmdData, au8CmdData);
		}
	}

	return bTxData;
}

#endif
