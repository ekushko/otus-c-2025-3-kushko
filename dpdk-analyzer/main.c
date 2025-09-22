#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <inttypes.h>
#include <signal.h>

#include <rte_eal.h>
#include <rte_ethdev.h>
#include <rte_mbuf.h>
#include <rte_ether.h>
#include <rte_ip.h>
#include <rte_tcp.h>
#include <rte_udp.h>
#include <rte_icmp.h>

#include "logger.h"

#define RX_RING_SIZE 1024
#define TX_RING_SIZE 1024
#define RX_RING_SIZE 1024
#define TX_RING_SIZE 1024
#define NUM_MBUFS 8191
#define MBUF_CACHE_SIZE 250
#define BURST_SIZE 32
#define LOG_MSGSTRLEN 2048
#define PROTO_STRLEN 20

static volatile bool force_quit = false;

typedef struct {
    uint64_t total_packets;
    uint64_t total_bytes;
    uint64_t eth_packets;
    uint64_t ip_packets;
    uint64_t tcp_packets;
    uint64_t udp_packets;
    uint64_t icmp_packets;
    uint64_t other_packets;
} traffic_stats;

static traffic_stats stats = {0};

Logger* logger = NULL;

// Обработчик сигнала для graceful shutdown
static void signal_handler(int signum) {
    if (signum == SIGINT || signum == SIGTERM) {
        printf("\nSignal %d received, preparing to exit...\n", signum);
        force_quit = true;
    }
}

void be32_to_ip_string(rte_be32_t be_ip, char *str, size_t str_size) {
    uint32_t host_ip = ntohl(be_ip);
    snprintf(str, str_size, "%u.%u.%u.%u",
        (host_ip >> 24) & 0xFF,
        (host_ip >> 16) & 0xFF,
        (host_ip >> 8) & 0xFF,
        host_ip & 0xFF);
}

static int port_init(uint16_t port, struct rte_mempool* mbuf_pool) {
    struct rte_eth_conf port_conf = {              // структура, используемая для настройки порта Ethernet
        .rxmode = {                                // структура, используемая для настройки функций приема порта Ethernet
            .max_lro_pkt_size = RTE_ETHER_MAX_LEN, // максимальный размер агрегированного (Large Receive Offload / LRO) пакета
        },
    };

    int ret;
    uint16_t nb_rxd = RX_RING_SIZE; // Размер RX-кольца (приема)
    uint16_t nb_txd = TX_RING_SIZE; // Размер TX-кольца (передача)

    // настройка устройства Ethernet
    // (порт, кол-во очередей на прием, кол-во очередей на передачу, настройки порта)
    ret = rte_eth_dev_configure(port, 1, 1, &port_conf);
    if (ret != 0) {
        return ret;
    }

    // выделение памяти и инициализация очереди на прием для устройства Ethernet
    // (порт, индекс очереди, число дескрипторов для кольца приема, сокет,
    // конфигурация очереди на прием, пул буфера сетевой памяти)
    ret = rte_eth_rx_queue_setup(port, 0, nb_rxd, rte_eth_dev_socket_id(port), NULL, mbuf_pool);
    if (ret != 0) {
        return ret;
    }

    // запуск устройства Ethernet
    ret = rte_eth_dev_start(port);
    if (ret < 0) {
        return ret;
    }

    // включение неразборчивого режима (прием любых пакетов)
    rte_eth_promiscuous_enable(port);

    return 0;
}

static void analyze_packet(struct rte_mbuf *pkt) {
    char log_msg[LOG_MSGSTRLEN];

    struct rte_ether_hdr *eth_hdr;
    struct rte_ipv4_hdr *ip_hdr;
    struct rte_tcp_hdr *tcp_hdr;
    struct rte_udp_hdr *udp_hdr;

    stats.total_packets++;
    stats.total_bytes += pkt->pkt_len;

    eth_hdr = rte_pktmbuf_mtod(pkt, struct rte_ether_hdr *);

    if (eth_hdr->ether_type == rte_cpu_to_be_16(RTE_ETHER_TYPE_IPV4)) {
        stats.ip_packets++;
        ip_hdr = (struct rte_ipv4_hdr *)(eth_hdr + 1);

        char src_addr[INET_ADDRSTRLEN];
        int  src_port = 0;
        char dst_addr[INET_ADDRSTRLEN];
        int  dst_port = 0;
        char proto[PROTO_STRLEN];

        be32_to_ip_string(ip_hdr->src_addr, src_addr, sizeof(src_addr));
        be32_to_ip_string(ip_hdr->dst_addr, dst_addr, sizeof(dst_addr));

        switch (ip_hdr->next_proto_id) {
            case IPPROTO_TCP:
                stats.tcp_packets++;
                tcp_hdr = (struct rte_tcp_hdr *)(ip_hdr + 1);
                src_port = tcp_hdr->src_port;
                dst_port = tcp_hdr->dst_port;
                snprintf(log_msg, LOG_MSGSTRLEN, "%s:%d > %s:%d proto TCP", src_addr, src_port, dst_addr, dst_port);
                break;

            case IPPROTO_UDP:
                stats.udp_packets++;
                udp_hdr = (struct rte_udp_hdr *)(ip_hdr + 1);
                src_port = udp_hdr->src_port;
                dst_port = udp_hdr->dst_port;
                snprintf(log_msg, LOG_MSGSTRLEN, "%s:%d > %s:%d proto UDP", src_addr, src_port, dst_addr, dst_port);
                break;

            case IPPROTO_ICMP:
                stats.icmp_packets++;
                snprintf(log_msg, LOG_MSGSTRLEN, "%s:%d > %s:%d proto ICMP", src_addr, src_port, dst_addr, dst_port);
                break;

            default:
                stats.other_packets++;
                snprintf(log_msg, LOG_MSGSTRLEN, "%s:%d > %s:%d proto ???", src_addr, src_port, dst_addr, dst_port);
                break;
        }        
    } else {
        stats.other_packets++;
        snprintf(log_msg, LOG_MSGSTRLEN, "???");
    }

    logger_info(logger, log_msg);
}

// Вывод статистики
static void print_stats(void) {
    printf("\n=== Traffic Statistics ===\n");
    printf("Total packets: %"PRIu64"\n", stats.total_packets);
    printf("Total bytes: %"PRIu64"\n", stats.total_bytes);
    printf("IP packets: %"PRIu64"\n", stats.ip_packets);
    printf("TCP packets: %"PRIu64"\n", stats.tcp_packets);
    printf("UDP packets: %"PRIu64"\n", stats.udp_packets);
    printf("ICMP packets: %"PRIu64"\n", stats.icmp_packets);
    printf("Other packets: %"PRIu64"\n", stats.other_packets);
    printf("==========================\n");
}

int main(int argc, char **argv) {
    logger = logger_init("log.txt");

    int ret;
    uint16_t port_id;
    struct rte_mempool *mbuf_pool;
    struct rte_mbuf *bufs[BURST_SIZE];
    uint16_t nb_rx;

    // Инициализация EAL
    ret = rte_eal_init(argc, argv);
    if (ret < 0) {
        rte_exit(EXIT_FAILURE, "Error with EAL initialization\n");
    }

    // Проверяем количество портов
    if (rte_eth_dev_count_avail() == 0) {
        rte_exit(EXIT_FAILURE, "No Ethernet ports found\n");
    }

    port_id = 0; // Используем первый доступный порт

    // Создаем memory pool
    mbuf_pool = rte_pktmbuf_pool_create("MBUF_POOL", NUM_MBUFS,
                                       MBUF_CACHE_SIZE, 0,
                                       RTE_MBUF_DEFAULT_BUF_SIZE,
                                       rte_socket_id());

    if (mbuf_pool == NULL) {
        rte_exit(EXIT_FAILURE, "Cannot create mbuf pool\n");
    }

    // Инициализируем порт
    if (port_init(port_id, mbuf_pool) != 0) {
        rte_exit(EXIT_FAILURE, "Cannot init port %"PRIu16"\n", port_id);
    }

    // Устанавливаем обработчик сигналов
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    printf("Traffic analyzer started on port %d. Press Ctrl+C to stop.\n", port_id);

    // Основной цикл обработки пакетов
    while (!force_quit) {
        // Получаем пакеты
        nb_rx = rte_eth_rx_burst(port_id, 0, bufs, BURST_SIZE);

        if (nb_rx == 0) {
            continue;
        }

        // Обрабатываем каждый пакет
        for (int i = 0; i < nb_rx; i++) {
            analyze_packet(bufs[i]);
            rte_pktmbuf_free(bufs[i]);
        }
    }

    // Очистка
    printf("Stopping traffic analyzer...\n");
    rte_eth_dev_stop(port_id);
    rte_eth_dev_close(port_id);

    print_stats(); // Финальная статистика

    logger_free(logger);

    return 0;
}
