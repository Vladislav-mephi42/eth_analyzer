import asyncio
import json
import os

from dotenv import load_dotenv
from web3 import AsyncWeb3
from web3.providers import AsyncHTTPProvider


async def get_wallet_info(w3: AsyncWeb3, address: str) -> dict:
    address = w3.to_checksum_address(address)
    nonce, balance = await asyncio.gather(
        w3.eth.get_transaction_count(address), w3.eth.get_balance(address)
    )
    return {
        "address": address,
        "nonce": nonce,
        "balance": float(w3.from_wei(balance, "ether")),
    }


async def get_transactions(w3: AsyncWeb3, address: str, max_count: int) -> list:
    address = w3.to_checksum_address(address)
    hex_count = f"0x{max_count:x}"
    categories = ["internal", "external", "erc20"]

    params_from = [
        {
            "fromAddress": address,
            "maxCount": hex_count,
            "category": categories,
            "order": "desc",
            "withMetadata": True,
        }
    ]

    response = await w3.provider.make_request("alchemy_getAssetTransfers", params_from)

    transfers = []

    if isinstance(response, dict) and "result" in response:
        result_data = response["result"]
        if result_data and "transfers" in result_data:
            transfers.extend(result_data["transfers"])

    transfers.sort(key=lambda t: int(t["blockNum"], 16), reverse=True)

    return transfers[:max_count]


async def get_transactions_with_neighbours(
    w3: AsyncWeb3, address: str, max_count: int
) -> dict:
    address = w3.to_checksum_address(address)
    target_node, transactions = await asyncio.gather(
        get_wallet_info(w3, address), get_transactions(w3, address, max_count)
    )

    neighbours = set()
    edges = []

    for tx in transactions:
        if tx and tx.get("to") and tx.get("from"):
            sender = w3.to_checksum_address(tx.get("from"))
            receiver = w3.to_checksum_address(tx.get("to"))

            if sender == address:
                neighbours.add(receiver)
                # print("\n++++++++++++++++++++++++++++++++++++++++\n")
            elif receiver == address:
                neighbours.add(sender)
                # print("\n++++++++++++++++++++++++++++++++++++++++\n")
            else:
                print("\n??????????????????\n?????????????")

            edge = {
                "from": sender,
                "to": receiver,
                "value": tx.get("value"),
                "timestamp": tx.get("metadata", {}).get("blockTimestamp"),
                "hash": tx.get("hash"),
            }
            edges.append(edge)

    neighbour_tasks = [get_wallet_info(w3, addr) for addr in neighbours]
    neighbours_data = await asyncio.gather(*neighbour_tasks)

    res = {
        "target_node": target_node,
        "edges": edges,
        "neighbours_data": neighbours_data,
    }
    return res


async def producer(w3, address, max_height, max_deep, queue):
    neighbours = [address]
    for i in range(max_deep):
        tasks = [
            get_transactions_with_neighbours(w3, n, max_height) for n in neighbours
        ]
        res = await asyncio.gather(*tasks)
        neighbours = []
        for n1 in res:
            await queue.put(n1)
            for n2 in n1.get("neighbours_data"):
                neighbours.append(n2.get("address"))


async def consumer(queue):
    counter = 0
    edge_counter = 0
    while True:
        item = await queue.get()
        print(
            f"\n\n[CONSUMED BY CONSUMER] NONSE ==={(item.get('target_node')).get('nonce')} "
        )

        counter += 1
        edge_counter += len(item.get("edges"))

        print("\n Counter == ", end="")
        print(counter, "\n\n")
        print("\n EdgeCounter == ", end="")
        print(edge_counter, "\n\n")
        for i in item.get("edges"):
            print((str(i.get("to")))[2:6], end="  ")
        print()
        queue.task_done()


async def global_func(address):
    load_dotenv()
    RLC_URL = os.getenv("ALCHEMY_HTTP_URL")
    if not RLC_URL:
        raise ValueError("Error: ALCHEMY_HTTP_URL variable not found in .env file!")

    w3 = AsyncWeb3(AsyncHTTPProvider(RLC_URL))

    if not (await w3.is_connected()):
        raise ValueError(
            f"Error: bad ALCHEMY_HTTP_URL, connection corrupted: {RLC_URL}"
        )

    print("[PARSER] connection was successful")

    queue = asyncio.Queue()

    producer_task = asyncio.create_task(producer(w3, address, 2, 6, queue))
    consumer_task = asyncio.create_task(consumer(queue))

    await producer_task
    await queue.join()

    consumer_task.cancel()


if __name__ == "__main__":
    asyncio.run(global_func("0xd8dA6BF26964aF9D7eEd9e03E53415D37aA96045"))
