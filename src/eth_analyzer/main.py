import os

from dotenv import load_dotenv
from web3 import Web3

from .net_client import NetClient

load_dotenv()
RPC_URL = os.getenv("ALCHEMY_HTTP_URL")

if not RPC_URL:
    raise ValueError("Error: ALCHEMY_HTTP_URL variable not found in .env file!")


w3 = Web3(Web3.HTTPProvider(RPC_URL))


def start():

    if not w3.is_connected():
        print("Failed to connect to the Ethereum network.")
        return

    print("Successfully connected! Fetching the latest block...")

    block = w3.eth.get_block("latest", full_transactions=True)

    block_number = block["number"]
    transactions = block["transactions"]

    print(f"\n[ Block #{block_number} ] Total transactions: {len(transactions)}")
    print("=" * 50)
    with NetClient("127.0.0.1", 7009) as client:
        for new_tx in transactions[:5]:
            tx_hash = new_tx["hash"].hex()
            from_address = new_tx["from"]
            to_address = new_tx["to"]
            value_wei = new_tx["value"]
            value_eth = w3.from_wei(value_wei, "ether")
            tx = {}
            tx["hash"] = "INET" + tx_hash
            tx["from"] = "INET" + from_address
            tx["to"] = "INET" + to_address

            client.send_transaction(tx)
            tx = client.get_transaction()

            print(f"Hash:  0x{tx['hash']}")
            print(f"From:  {tx['from']}")
            print(f"To:    {tx['to']}")
            print(f"Value: {value_eth} ETH")
            print("-" * 50)


if __name__ == "__main__":
    start()
