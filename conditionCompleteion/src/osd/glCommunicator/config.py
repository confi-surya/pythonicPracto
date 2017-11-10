GL_FS_PATH = "/export/.osd_meta_config"
GL_FILE_BASE = "osd_global_leader_information"

CONNECTION_RETRY = 3
TIMEOUT = 10
MAX_FILE_READ_SIZE = 4096

NODE_STATUS_MAP = {
                10: "RUNNING",
                20: "STOPPING",
                30: "STOP",
                40: "REGISTERED",
                50: "FAILED",
                60: "RECOVERED",
                70: "RETIRING",
                80: "RETIRED",
                100: "INVALID_NODE"
                }
class IoType:
    EVENTIO = "EventIo"
    SOCKETIO = "SocketIo"
    ASYNCIO = "AsyncIo"
    HTTPIO = "HttpIo"


