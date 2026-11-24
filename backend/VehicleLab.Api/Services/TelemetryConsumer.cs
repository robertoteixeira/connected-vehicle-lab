using Azure.Messaging.EventHubs.Consumer;

namespace VehicleLab.Api.Services;

public sealed class TelemetryConsumer : BackgroundService
{
    private readonly IConfiguration configuration;
    private readonly ILogger<TelemetryConsumer> logger;

    public TelemetryConsumer(IConfiguration configuration, ILogger<TelemetryConsumer> logger)
    {
        this.configuration = configuration;
        this.logger = logger;
    }

    protected override async Task ExecuteAsync(CancellationToken stoppingToken)
    {
        var connectionString = configuration["IoTHub:EventHubConnectionString"];

        if (string.IsNullOrWhiteSpace(connectionString))
        {
            throw new InvalidOperationException("Missing IoTHub:EventHubConnectionString configuration");
        }

        await using var consumer = new EventHubConsumerClient(
            EventHubConsumerClient.DefaultConsumerGroupName,
            connectionString
        );

        logger.LogInformation("Starting Azure telemetry consumer");

        try
        {
            await foreach (var received in consumer.ReadEventsAsync(
                startReadingAtEarliestEvent: false,
                cancellationToken: stoppingToken))
            {
                logger.LogInformation(
                    "Telemetry received: {Payload}",
                    received.Data.EventBody.ToString()
                );
            }
        }
        catch (OperationCanceledException) when (stoppingToken.IsCancellationRequested)
        {
            logger.LogInformation("Azure telemetry consumer stopped");
        }
    }
}
