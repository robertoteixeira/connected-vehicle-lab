using VehicleLab.Api.Services;

var builder = WebApplication.CreateBuilder(args);
builder.Services.AddHostedService<TelemetryConsumer>();
var app = builder.Build();

app.MapGet("/health", () => Results.Ok(new
{
    status = "ok"
}));

app.Run();
