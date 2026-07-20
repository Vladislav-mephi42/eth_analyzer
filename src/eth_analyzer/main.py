import os

from dotenv import load_dotenv
from web3 import Web3

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

    for tx in transactions[:5]:
        tx_hash = tx["hash"].hex()
        from_address = tx["from"]
        to_address = tx["to"]
        value_wei = tx["value"]
        value_eth = w3.from_wei(value_wei, "ether")

        print(f"Hash:  0x{tx_hash}")
        print(f"From:  {from_address}")
        print(f"To:    {to_address}")
        print(f"Value: {value_eth} ETH")
        print("-" * 50)


if __name__ == "__main__":
    start()
